#

PARTSORTSRCS += misc.c stack.c 
PARTSORTSRCS += ElementaryClasses.c
PARTSORTSRCS += box_vector.c int_vector.c rule_vector.c ll_vector.c u_multiset.c
PARTSORTSRCS += IntervalUtilities.c red_black_tree.c 
PARTSORTSRCS += SortableRulesetPartitioner.c OptimizedMITree.c 
PARTSORTSRCS += PartitionSort.c

LDFLAGS+= -lpcap
