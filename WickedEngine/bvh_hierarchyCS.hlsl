#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader will construct the BVH from sorted cluster morton codes.
//	Output is a list of continuous BVH tree nodes in memory: [parentIndex, leftChildNodeIndex, rightChildNodeIndex]. Additionally, we will reset the BVH Flag Buffer (used for AABB propagation step)
//	The output node is a leaf node if: leftChildNodeIndex == rightChildNodeIndex == 0
//	Else the output node is an intermediate node
//	Also, we know that intermediate nodes start at arrayIndex == 0 (starting with root node)
//	Also, we know that leaf nodes will start at arrayIndex == clusterCount -1 (and they will correspond to a single cluster, which is indexable by clusterIndexBuffer later)

// Using the Karras's 2012 parallel BVH construction algorithm outlined 
// in "Maximizing Parallelism in the Construction of BVHs, Octrees,
// and k-d Trees"

RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(clusterMortonBuffer, float, TEXSLOT_ONDEMAND1); // morton buffer is float because sorting is written for floats!

RWSTRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, 0);
RWSTRUCTUREDBUFFER(bvhFlagBuffer, uint, 1);

int CountLeadingZeroes(uint num)
{
	return 31 - firstbithigh(num);
}

int GetLongestCommonPrefix(uint indexA, uint indexB, uint elementCount)
{
	if (indexA >= elementCount || indexB >= elementCount)
	{
		return -1;
	}
	else
	{
		uint mortonCodeA = uint(clusterMortonBuffer[indexA]);
		uint mortonCodeB = uint(clusterMortonBuffer[indexB]);
		if (mortonCodeA != mortonCodeB)
		{
			return CountLeadingZeroes(mortonCodeA ^ mortonCodeB);
		}
		else
		{
			// TODO: Technically this should be primitive ID
			return CountLeadingZeroes(indexA ^ indexB) + 31;
		}
	}
}

uint2 DetermineRange(uint idx, uint elementCount)
{
	int d = GetLongestCommonPrefix(idx, idx + 1, elementCount) - GetLongestCommonPrefix(idx, idx - 1, elementCount);
	d = clamp(d, -1, 1);
	int minPrefix = GetLongestCommonPrefix(idx, idx - d, elementCount);

	// TODO: Consider starting this at a higher number
	int maxLength = 2;
	while (GetLongestCommonPrefix(idx, idx + maxLength * d, elementCount) > minPrefix)
	{
		maxLength *= 4;
	}

	int length = 0;
	for (int t = maxLength / 2; t > 0; t /= 2)
	{
		if (GetLongestCommonPrefix(idx, idx + (length + t) * d, elementCount) > minPrefix)
		{
			length = length + t;
		}
	}

	int j = idx + length * d;
	return uint2(min(idx, j), max(idx, j));
}

int FindSplit(int first, uint last, uint elementCount)
{
	int commonPrefix = GetLongestCommonPrefix(first, last, elementCount);
	int split = first;
	int step = last - first;

	do
	{
		step = (step + 1) >> 1;
		int newSplit = split + step;

		if (newSplit < last)
		{
			int splitPrefix = GetLongestCommonPrefix(first, newSplit, elementCount);
			if (splitPrefix > commonPrefix)
				split = newSplit;
		}
	} while (step > 1);

	return split;
}



[numthreads(BVH_HIERARCHY_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint idx = DTid.x;
	const uint clusterCount = clusterCounterBuffer.Load(0);

	if (idx < clusterCount - 1)
	{
		uint2 range = DetermineRange(idx, clusterCount);
		uint first = range.x;
		uint last = range.y;

		uint split = FindSplit(first, last, clusterCount);

		uint internalNodeOffset = 0;
		uint leafNodeOffset = clusterCount - 1;
		uint childAIndex;
		if (split == first)
			childAIndex = leafNodeOffset + split;
		else
			childAIndex = internalNodeOffset + split;

		uint childBIndex;
		if (split + 1 == last)
			childBIndex = leafNodeOffset + split + 1;
		else
			childBIndex = internalNodeOffset + split + 1;

		// write to parent:
		bvhNodeBuffer[idx].LeftChildIndex = childAIndex;
		bvhNodeBuffer[idx].RightChildIndex = childBIndex;
		// write to children:
		bvhNodeBuffer[childAIndex].ParentIndex = idx;
		bvhNodeBuffer[childBIndex].ParentIndex = idx;

		// Reset bvh node flag (only internal nodes):
		bvhFlagBuffer[idx] = 0;
	}
}
