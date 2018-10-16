#include "rule_reader.h"

static bool IsPower2(unsigned int x) {
	return ((x - 1) & x) == 0;
}

static bool IsPrefix(unsigned int low, unsigned int high) {
	unsigned int diff = high - low;

	return ((low & high) == low) && IsPower2(diff + 1);
}

static unsigned int PrefixLength(unsigned int low, unsigned int high) {
	unsigned int x = high - low;
	int lg = 0;
	for (; x; x >>= 1) lg++;
	return 32 - lg;
}

static void ParseRange(unsigned *range, const char *text) {
	char *token = strtok(text, ":");
	// to obtain interval
	range[0] = (unsigned)atoi(token);
	token = strtok(NULL, ":");
	range[1] = (unsigned)atoi(token);
	if (range[0] > range[1]) {
		printf("Problematic range: %u-%u\n", range[0], range[1]);
	}
}

rule_vector * ReadFilterFileMSU(const char *filename)
{
	rule_vector *rules = rule_vector_init(); 
	FILE * fp = NULL;
	char * line = NULL;
	size_t len = 0;
	size_t read = 0;
	int dim = 5;
	int priority = 0;

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Failed to read file\n");
		return rules;
	}
	read = getline(&line, &len, fp);
	read = getline(&line, &len, fp);
	//strtok(line, ",");
	//while(strtok(NULL, ",")){ dim++; }

	read = getline(&line, &len, fp);
	//vector<string> parts = split(content, ',');
	//vector<array<unsigned int, 2>> bounds(parts.size());
	//for (size_t i = 0; i < parts.size(); i++) {
	//	ParseRange(bounds[i], parts[i]);
	//	//printf("[%u:%u] %d\n", bounds[i][LOW], bounds[i][HIGH], PrefixLength(bounds[i][LOW], bounds[i][HIGH]));
	//}
	while ((read = getline(&line, &len, fp)) != -1) {
		// 5 fields: sip, dip, sport, dport, proto = 0 (with@), 1, 2 : 4, 5 : 7, 8
		rule *temp_rule = rule_init(dim);
		char **split_comma = (char **)safe_malloc(sizeof(char *) * (dim + 1));	
		char *token = strtok(line, ",");
		for(int i = 0; i < dim + 1; i++){
			split_comma[i] = token;
			token = strtok(NULL, ",");
		}
		// ignore priority at the end
		for (size_t i = 0; i < dim; i++)
		{
			ParseRange(&temp_rule->fields[i].range, split_comma[i]);
			if (IsPrefix(temp_rule->fields[i].range[0], temp_rule->fields[i].range[1])) {
				temp_rule->prefix_length[i] = PrefixLength(temp_rule->fields[i].range[0], temp_rule->fields[i].range[1]);
			}
			//if ((i == FieldSA || i == FieldDA) & !IsPrefix(temp_rule.range[i][LOW], temp_rule.range[i][HIGH])) {
			//	printf("Field is not a prefix!\n");
			//}
			//if (temp_rule.range[i][LOW] < bounds[i][LOW] || temp_rule.range[i][HIGH] > bounds[i][HIGH]) {
			//	printf("rule out of bounds!\n");
			//}
		}
		temp_rule->priority = priority++;
		temp_rule->tag = atoi(split_comma[dim]);
		rule_vector_push_back(rules, temp_rule);
		free(split_comma);
	}
	rule **rules_a = rule_vector_to_array(rules);
	for(int i = 0; i < rule_vector_size(rules); i++){
		rules_a[i]->priority = rule_vector_size(rules) - rules_a[i]->priority;
	}

	fclose(fp);
	if (line)
		free(line);

	return rules;
}
