#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int TABLE_SIZE = 20;
int OLD_TABLE_SIZE = 20;
int nr_of_elements;
char delim[100] = "\t []<>*%%!&|^.:;)";
int paragraph;
typedef struct node {
	char *key;
	char *value;
	struct node *next;
} node;

typedef struct hashtable {
	node **buckets;
} hashtable;

int hash(char *key)
{
	long value = 0;
	int i = 0;
	int key_len = strlen(key);

	/*do several rounds of multiplication*/
	for (i = 0; i < key_len; i++)
		value = value * 33 + key[i];

	/*make sure value is 0 <= value < TABLE_SIZE*/
	if (nr_of_elements > TABLE_SIZE / 2) {
		OLD_TABLE_SIZE = TABLE_SIZE;
		TABLE_SIZE *= 2;
	}
	
	value = value % TABLE_SIZE;

	return value;
}

node *ht_pair(char *key, char *value)
{
	/*allocate the new_elem*/
	node *new_elem = malloc(sizeof(node) * 1);

	if (new_elem == NULL)
		exit(12);

	new_elem->key = malloc(strlen(key) + 1);
	if (new_elem->key == NULL)
		exit(12);
	new_elem->value = malloc(strlen(value) + 1);
	if (new_elem->value == NULL)
		exit(12);
	/*copy the key and value in place*/
	strcpy(new_elem->key, key);
	strcpy(new_elem->value, value);

	/*next starts out null but may be set later on*/
	new_elem->next = NULL;

	return new_elem;
}

hashtable *ht_init(void)
{
	int i = 0;
	/*allocate table*/
	hashtable *hashtable = malloc(sizeof(hashtable) * 1);

	if (hashtable == NULL)
		exit(12);

	/*allocate table buckets*/
	hashtable->buckets = malloc(sizeof(node *) * TABLE_SIZE);
	if (hashtable->buckets == NULL)
		exit(12);
	/*set each to null (needed for proper operation)*/

	for (i = 0; i < TABLE_SIZE; i++)
		hashtable->buckets[i] = NULL;

	return hashtable;
}

void ht_set(hashtable *hashtable, char *key, char *value)
{
	int slot = hash(key);
	int i;
	node *new_elem;
	node *prev;

	nr_of_elements++;
	if (OLD_TABLE_SIZE != TABLE_SIZE) {
		hashtable->buckets =
			(node **)realloc(hashtable->buckets, TABLE_SIZE * sizeof(node *));

		for (i = OLD_TABLE_SIZE; i < TABLE_SIZE; i++)
			hashtable->buckets[i] = NULL;

		OLD_TABLE_SIZE = TABLE_SIZE;
	}

	/*try to look up an new_elem set*/
	new_elem = hashtable->buckets[slot];

	/*no new_elem means slot empty, insert immediately*/
	if (new_elem == NULL)
		hashtable->buckets[slot] = ht_pair(key, value);
	else {

		while (new_elem != NULL) {
			/*check key*/
			if (strcmp(new_elem->key, key) == 0)
				strcpy(new_elem->value, value);

			else {
				/*walk to next*/
				prev = new_elem;
				new_elem = prev->next;
			}
		}

		/*end of chain reached without a match, add new*/
		prev->next = ht_pair(key, value);
	}
}

char *ht_get(hashtable *hashtable, char *key)
{
	int slot = hash(key);
	node *new_elem = hashtable->buckets[slot];

	if (new_elem == NULL)
		return NULL;

	while (new_elem != NULL) {
		/*return value if found*/
		if (strcmp(new_elem->key, key) == 0)
			return new_elem->value;

		new_elem = new_elem->next;
	}

	/*reaching here means there were >= 1 buckets but no key match*/
	return NULL;
}

void ht_pop(hashtable *hashtable, char *key)
{
	int slot = hash(key);
	node *new_elem = hashtable->buckets[slot];
	node *prev;
	int i = 0;

	if (new_elem == NULL)
		return;

	while (new_elem != NULL) {

		if (strcmp(new_elem->key, key) == 0) {
			/*first item and no next new_elem*/
			if (new_elem->next == NULL && i == 0)
				hashtable->buckets[slot] = NULL;

			/*first item with a next new_elem*/
			if (new_elem->next != NULL && i == 0)
				hashtable->buckets[slot] = new_elem->next;

			/*last item*/
			if (new_elem->next == NULL && i != 0)
				prev->next = NULL;

			/*middle item*/
			if (new_elem->next != NULL && i != 0)
				prev->next = new_elem->next;

			/*free the deleted new_elem*/
			free(new_elem->key);
			free(new_elem->value);
			free(new_elem);
			break;
		}
		prev = new_elem;
		new_elem = prev->next;
		i++;
	}
}

void ht_free(hashtable *hashtable)
{
	node *new_elem;
	node *tmp;
	int i;
	for (i = 0; i < TABLE_SIZE; i++) {
		new_elem = hashtable->buckets[i];

		while (new_elem != NULL) {
			tmp = new_elem;
			free(tmp->key);
			free(tmp->value);
			new_elem = new_elem->next;
			free(tmp);
		}
	}
	free(hashtable->buckets);
	free(hashtable);
}
void process_file(FILE *input, hashtable *ht, char input_locations[10][100],
				  int input_location_index, FILE *output)
{
	
	char buf[256];
	char aux[256];
	char aux2[256];
	char result[256] = "";
	char *token;
	int got_if = 0;
	int got_ifdef = 0;
	int ifdef_mode = 0;
	int skip = 0;

	char key[32];
	char value[32];
	char aux3[32];
	char aux4[32] = "";
	int i;
	FILE *to_open;
	int found_file = 0;
	char file_location[100] = "";
	char replaced_delim[3];
	int ok = 1;
	char nonnumbers[256] =
		"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGH"
		"JKLZXCVBNM[]{}|;',./<>?!@#$^&*()_+=-";
	int int_result;
	int nr_ghil = 0;
	char *ghil;

	while (fgets(buf, 1000, input) != NULL) {

		buf[strcspn(buf, "\n")] = 0;

		strcpy(result, "");
		strcpy(aux, buf);
		strcpy(aux2, buf);

		if (strstr(aux, "undef") && skip == 0 && aux[0] == '#') {
			token = strtok(aux, " ");
			token = strtok(NULL, " ");
			ht_pop(ht, token);
			continue;
		}
		if (strstr(aux, "#define") && skip == 0 && aux[0] == '#') {
			token = strtok(aux, " ");
			token = strtok(NULL, " ");
			
			strcpy(key, token);
			token = strtok(NULL, "\n");
			if (token == NULL) {
				strcpy(value, "");
				ht_set(ht, key, value);
				continue;
			} else {
				strcpy(value, token);

				while (ht_get(ht, value) != NULL) 
					strcpy(value, ht_get(ht, value));
				
				strcpy(aux4, "");
				strcpy(aux3, value);
				token = strtok(aux3, " ");
				while (token != NULL) {
					if (ht_get(ht, token) != NULL) {
						strcat(aux4, ht_get(ht, token));
						//printf("%s!\n",aux4);
						//aux4[strlen(aux4)-1]=0;
						

					}
						
						
					else {
						strcat(aux4, token);
						//strcat(aux4, " ");
					}
					strcat(aux4, " ");	
					token = strtok(NULL, " ");
				}
				aux4[strlen(aux4)-1]=0;
				if (!strcmp(aux4, value))
					ht_set(ht, key, value);
				else
					ht_set(ht, key, aux4);
				continue;
			}
		} else if (strstr(aux, "#include")) {

			found_file = 0;
			token = strtok(aux, "\"");

			token = strtok(NULL, "\"");

			to_open = fopen(token, "r");

			if (to_open == NULL) {

				strcpy(file_location, "");
				strcat(file_location, "_test/inputs/");

				strcat(file_location, token);

				to_open = fopen(file_location, "r");
				if (to_open != NULL)
					found_file = 1;

				if (!found_file)
					for (i = 0; i < input_location_index; i++) {
						strcpy(file_location, "");

						strcat(file_location, input_locations[i]);
						strcat(file_location, "/");
						strcat(file_location, token);
						to_open = fopen(file_location, "r");

					if (to_open != NULL) {
						found_file = 1;
						break;
					}
					}
			} else
				found_file = 1;

			if (!found_file) {
				ht_free(ht);
				fclose(input);
				fclose(output);
				exit(-1);
			} else {
				found_file = 0;
				process_file(to_open, ht, input_locations, input_location_index,
							 output);
				fclose(to_open);
			}
		} else if (strstr(aux, "#ifdef")) {

			token = strtok(aux, " ");
			token = strtok(NULL, " ");
			strcpy(key, token);
			ifdef_mode = 1;
			if (ht_get(ht, key) == NULL)
				skip = 1;
			else
				got_ifdef = 1;
		} else if (strstr(aux, "#ifndef")) {
			token = strtok(aux, " ");
			token = strtok(NULL, " ");
			strcpy(key, token);
			ifdef_mode = 1;
			if (ht_get(ht, key) != NULL)
				skip = 1;
			else
				got_ifdef = 1;
		} else if (!strcmp(aux, "#else") && ifdef_mode == 1) {
			if (got_ifdef)
				skip = 1;
			else
				got_ifdef = 1;
		} else {
			token = strtok(aux, delim);

			if (token != NULL) {
				replaced_delim[0] = aux2[token - aux + strlen(token)];
				replaced_delim[1] = '\0';
			} else
				replaced_delim[0] = '\0';

			if (skip == 1) {
				skip = 0;
				continue;
			}
			while (token != NULL) {
				if (!strcmp(token, "#endif")) {
					ifdef_mode = 0;
					got_if = 0;
					got_ifdef = 0;
					skip = 0;
					break;
				} else if (strstr(token, "if") && strstr(token, "#")) {
					if (got_if)
						skip = 1;
					else {
						token = strtok(NULL, " ");

						if (ht_get(ht, token) != NULL ||
							strcmp(token, "0")) {
							ok = 1;

							if (ht_get(ht, token) == NULL) {
								for (i = 0; i < strlen(token); i++)
									if (strchr(nonnumbers, token[i])) {
										skip = 1;
										ok = 0;
										break;
									}
								if (ok == 1) {
									int_result = atoi(token);
									if (int_result == 0)
										skip = 1;
									else
										got_if = 1;
								}
							} else {
								strcpy(value, ht_get(ht, token));

								for (i = 0; i < strlen(value); i++) {
									if (strchr(nonnumbers, value[i])) {
										skip = 1;
										ok = 0;
									}
								}
								if (ok == 1) {
									int_result = atoi(value);
									if (int_result == 0)
										skip = 1;
									else
										got_if = 1;
								}
							}
						} else
							skip = 1;
					}
					token = strtok(NULL, " ");
				} else if (strstr(token, "else") && strstr(token, "#")) {
					if (got_if)
						skip = 1;
					else
						got_if = 1;
					token = strtok(NULL, " ");
					break;
				} else {
					nr_ghil = 0;
					ghil = strchr(token, '"');

					if (ghil != NULL) {
						if (token != NULL)
							strcat(result, token);
						for (i = 0; i < strlen(token); i++) {
							if (token[i] == '"')
								nr_ghil++;
						}
						if (nr_ghil == 1) {
							strcat(result, replaced_delim);
							token = strtok(NULL, "\"");
							strcat(result, token);
							strcat(result, "\"");
						}
					} else {
						if (ht_get(ht, token) != NULL) {
							strcat(result, ht_get(ht, token));
							strcat(result, replaced_delim);
						} else {
							strcat(result, token);
							strcat(result, replaced_delim);
						}
					}

					if (result[strlen(result) - 1] == ')' &&
						result[strlen(result) - 2] != '(')
						strcat(result, ";");

					token = strtok(NULL, delim);

					if (token != NULL) {
						replaced_delim[0] =
							aux2[token - aux + strlen(token)];
					} else {
						if (result[strlen(result) - 1] == '"')
							strcat(result, ");");
					}
				}
			}
		}
			if (strcmp(result, "") && skip == 0) {
				if (output != NULL)
					fprintf(output, "%s\n", result);
			else
				printf("%s\n",result);
		}
	}
}
int main(int argc, char **argv)
{
	FILE *input, *output;
	hashtable *ht = ht_init();
	
	char input_location[10][100];
	int input_location_index = 0;
	char input_file[100] = "";
	char output_file[100] = "";
	int d_alone = 0;
	int i_alone = 0;
	int o_alone = 0;
	int i;
	char *token;
	char symbol[32];
	char mapping[32];
	char file_location[100];


	for (i = 1; i < argc; i++) {
		if (strstr(argv[i], "-D")) {
			if (strlen(argv[i]) == 2) {
				
				d_alone = 1;
			} else {
				strcpy(symbol, strtok(argv[i] + 2, "="));
				token = strtok(NULL, "=");
				if (token != NULL) {
					strcpy(mapping, token);
					ht_set(ht, symbol, mapping);
				} else 
					ht_set(ht, symbol, "");
			}
		} else if (d_alone == 1) {
			strcpy(symbol, strtok(argv[i], "="));
			d_alone = 0;
			token = strtok(NULL, "=");
			if(token ==NULL) 
				ht_set(ht, symbol, "");
			else {
				strcpy(mapping, token);
				ht_set(ht, symbol, mapping);
			}
	
		} else if (strstr(argv[i], "-I")) {
			if (strlen(argv[i]) == 2)
				i_alone = 1;
			else {
				strcpy(input_location[input_location_index], argv[i] + 2);
				input_location_index++;
			}
		} else if (i_alone == 1) {
			i_alone = 0;
			strcpy(input_location[input_location_index], argv[i]);
			input_location_index++;
		} else if (strstr(argv[i], "-o")) {
			if (strlen(argv[i]) == 2) 
				o_alone = 1;
			else
				strcpy(output_file, argv[i] + 2);
		} else if (o_alone == 1) {
			o_alone = 0;
			strcpy(output_file, argv[i]);
		} else {
			if (!strcmp(input_file, ""))
				strcpy(input_file, argv[i]);
			else if (!strcmp(output_file, ""))
				strcpy(output_file, argv[i]);
			else {
				ht_free(ht);
				exit(-1);
			}
		}
	}
	input = fopen(input_file, "r");
	if (input == NULL) {
		for (i = 0; i < input_location_index; i++) {
			strcpy(file_location, "");
			strcat(file_location, input_location[i]);
			strcat(file_location, "/");
			strcat(file_location, input_file);

			input = fopen(file_location, "r");
		}
	}
	if (strcmp(input_file, "") && input == NULL) {
		ht_free(ht);
		fclose(input);
		exit(-1);
	}
	if (input == NULL)
		input = stdin;
	output = fopen(output_file, "w");


	process_file(input, ht, input_location, input_location_index, output);

	ht_free(ht);
	fclose(input);
	if (output != NULL)
		fclose(output);
	return 0;
}
