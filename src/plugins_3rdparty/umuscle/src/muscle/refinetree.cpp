#include "muscle.h"
#include "msa.h"
#include "tree.h"
#include "profile.h"
#include <stdio.h>
#include "muscle_context.h"

void RefineTree(MSA &msa, Tree &tree)
	{
    MuscleContext *ctx = getMuscleContext();
    unsigned &g_uMaxTreeRefineIters = ctx->params.g_uMaxTreeRefineIters;
    CLUSTER &g_Cluster2 = ctx->params.g_Cluster2;
    DISTANCE &g_Distance2 =  ctx->params.g_Distance2;
    ROOT &g_Root2 = ctx->params.g_Root2;
    const char* &g_pstrDistMxFileName2 = ctx->params.g_pstrDistMxFileName2;
	const unsigned uSeqCount = msa.GetSeqCount();
	if (tree.GetLeafCount() != uSeqCount)
		Quit("Refine tree, tree has different number of nodes");

	if (uSeqCount < 3)
		return;

#if	DEBUG
	ValidateMuscleIds(msa);
	ValidateMuscleIds(tree);
#endif

	unsigned *IdToDiffsLeafNodeIndex = new unsigned[uSeqCount];
	unsigned uDiffsCount = uSeqCount;
	Tree Tree2;
	for (unsigned uIter = 0; uIter < g_uMaxTreeRefineIters; ++uIter)
		{
		TreeFromMSA(msa, Tree2, g_Cluster2, g_Distance2, g_Root2, g_pstrDistMxFileName2);

#if	DEBUG
		ValidateMuscleIds(Tree2);
#endif

		Tree Diffs;
		DiffTrees(Tree2, tree, Diffs, IdToDiffsLeafNodeIndex);

		tree.Copy(Tree2);

		const unsigned uNewDiffsNodeCount = Diffs.GetNodeCount();
		const unsigned uNewDiffsCount = (uNewDiffsNodeCount - 1)/2;

		if (0 == uNewDiffsCount || uNewDiffsCount >= uDiffsCount)
			{
			ProgressStepsDone();
			break;
			}
		uDiffsCount = uNewDiffsCount;

		MSA msa2;
		RealignDiffs(msa, Diffs, IdToDiffsLeafNodeIndex, msa2);

#if	DEBUG
		ValidateMuscleIds(msa2);
#endif

		msa.Copy(msa2);
		SetCurrentAlignment(msa);
		}

	delete[] IdToDiffsLeafNodeIndex;
	}
