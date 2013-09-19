/*
 * function_models.c
 *
 *  Created on: Jul 19, 2013
 *      Author: muath
 */

#include "stranger.h"
#include "stranger_lib_internal.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>


/*=====================================================================*/
/* Data structure for transition relation matrix
*/

void removeTransitionOnChar(char* transitions, char* charachter, int var, char** result, int* pSize);


struct transition_type{
	int from;
	int to;
	struct transition_type	*next;
};

struct transition_list_type{
	int count;
	struct	transition_type	*head;
	struct	transition_type	*tail;
};

struct transition_type *transition_new_it(int from, int to) {
	struct transition_type *tmp;
	tmp = (struct transition_type *) malloc(sizeof(struct transition_type));
	tmp->from = from;
	tmp->to = to;
	tmp->next = NULL;
	return tmp;
}

struct transition_list_type *transition_new_ilt() {
	struct transition_list_type *tmp;
	tmp = (struct transition_list_type *) malloc(sizeof(struct transition_list_type));
	tmp->count = 0;
	tmp->head = tmp->tail = NULL;
	return tmp;
}

struct transition_list_type *transition_add_data(struct transition_list_type *list, struct transition_type *data)
{
	if (data == NULL)
		return list;

	if (list == NULL)
		list = transition_new_ilt();
	if (list->count > 0) {
		list->tail->next = data;
		list->tail = list->tail->next;
		list->count++;
	} else {
		list->head = list->tail = data;
		list->count = 1;
	}
	return list;
}

int transition_check_value(list, from, to)
	struct transition_list_type *list;int from; int to; {
	struct transition_type *tmp = NULL;
	if (list != NULL)
		for (tmp = list->head; tmp != NULL; tmp = tmp->next)
			if (tmp->from == from && tmp->to == to)
				return 1;
	return 0;
}

struct transition_list_type *transition_remove_value(struct transition_list_type *list, int from, int to)
{
	struct transition_type *tmp = NULL;
	struct transition_type *del = NULL;
	if (transition_check_value(list, from, to) > 0) {
		for (tmp = list->head; tmp != NULL; tmp = tmp->next)
			if ((tmp->from == from && tmp->to == to) && (list->count == 1)) { //remove tmp
				list->count = 0;
				list->head = list->tail = NULL;
				free(tmp);
				return list;
			} else if ((tmp->from == from && tmp->to == to) && (list->head == tmp)) {
				list->count--;
				list->head = list->head->next;
				free(tmp);
				return list;
			} else if ((tmp->next != NULL) && (tmp->next->from == from && tmp->next->to == to)) {
				if (tmp->next->next == NULL) { //remove tail
					list->count--;
					list->tail = tmp;
					list->tail->next = NULL;
					free(tmp->next);
					return list;
				} else {
					list->count--;
					del = tmp->next;
					tmp->next = tmp->next->next;
					free(del);
					return list;
				}
			}
	}
	return list;
}

int transition_check_data(list, data)
	struct transition_list_type *list;struct transition_type *data; {
	struct transition_type *tmp = NULL;
	if ((list != NULL) && (data != NULL))
		for (tmp = list->head; tmp != NULL; tmp = tmp->next)
			if (tmp->from == data->from && tmp->to == data->to)
				return 1;
	return 0;
}


//no duplicate
struct transition_list_type *transition_enqueue(struct transition_list_type *list, int from, int to)
{
	if (!transition_check_value(list, from, to))
		list = transition_add_data(list, transition_new_it(from, to));
	return list;
}

struct transition_type *transition_dequeue(struct transition_list_type *list)
{
	struct transition_type *data = NULL;
	struct transition_type *i = NULL;
	if ((list == NULL) || (list->count == 0))
		return NULL;
	else
		data = list->head;
	if (list->count == 1) {
		list->count = 0;
		list->head = list->tail = NULL;
	} else {
		list->head = list->head->next;
		list->count--;
	}
	i = data;
	free(data);
	return i;
}

void transition_free_ilt(struct transition_list_type *list) {
	struct transition_type *tmp = transition_dequeue(list);
	while (tmp != NULL)
		tmp = transition_dequeue(list);
	free(list);
}

void transition_print_ilt(struct transition_list_type *list) {
	if (list == NULL){
		printf("list is empty.\n");
		return;
	}
	struct transition_type *tmp = list->head;
	while (tmp != NULL) {
		printf("-> from: %d, to:%d", tmp->from, tmp->to);
		tmp = tmp->next;
	}
}


DFA *dfaPreRightTrim(DFA *M, char c, int var, int *oldIndices) {
	DFA *result;
	paths state_paths, pp;
	trace_descr tp;
	int i, j, k;
	char *exeps;
	int *to_states;
	int sink;
	long max_exeps;
	char *statuces;
	int len;
	int ns = M->ns;
	int *indices;
	char* lambda = bintostr(c, var);

	len = var + 1;
	max_exeps = 1 << len; //maybe exponential
	sink = find_sink(M);
	assert(sink>-1);

	indices = allocateArbitraryIndex(len);

	char* symbol = (char *) malloc((len + 1) * sizeof(char));//len+1 since we need extra bit
	exeps = (char *) malloc(max_exeps * (len + 1) * sizeof(char));
	to_states = (int *) malloc(max_exeps * sizeof(int));
	statuces = (char *) malloc((ns + 1) * sizeof(char));

	strcpy(symbol, lambda); symbol[var] = '1'; symbol[len] = '\0';


	dfaSetup(ns, len, indices);
	for (i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);
		k = 0;
		while (pp) {
			if (pp->to != sink) {
				to_states[k] = pp->to;
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp
							= tp->next)
						;
					if (tp) {
						if (tp->value)
							exeps[k * (len + 1) + j] = '1';
						else
							exeps[k * (len + 1) + j] = '0';
					} else
						exeps[k * (len + 1) + j] = 'X';
				}
				exeps[k * (len + 1) + j] = '0';// for init state extrabit 1 goes to self loop for lambda. Everthing else goes to sink
				exeps[k * (len + 1) + len] = '\0';// if no extrabit will overwrite assign in prev line
				k++;
			}
			pp = pp->next;
		}
		kill_paths(state_paths);

		// if accept state create a self loop on lambda
		if (M->f[i] == 1){
			dfaAllocExceptions(k+1);
			dfaStoreException(i, symbol);
		}
		else
			dfaAllocExceptions(k);

		for (k--; k >= 0; k--)
			dfaStoreException(to_states[k], exeps + k * (len + 1));
		dfaStoreState(sink);

		if (M->f[i] == -1)
			statuces[i] = '-';
		else if (M->f[i] == 1)
			statuces[i] = '+';
		else
			statuces[i] = '-';// DO NOT USE don't care
	}

	statuces[ns] = '\0';
	DFA* tmpM = dfaBuild(statuces);
	result = dfaProject(tmpM, ((unsigned)var));
	dfaFree(tmpM); tmpM = NULL;
	tmpM = dfaMinimize(result);
	dfaFree(result);result = NULL;

	free(exeps);
	free(symbol);
	free(lambda);
	free(to_states);
	free(statuces);
	free(indices);

	return tmpM;
}



DFA *dfaPreLeftTrim(DFA *M, char c, int var, int *oldIndices) {
	DFA *result;
	paths state_paths, pp;
	trace_descr tp;
	int i, j, k;
	char *exeps;
	int *to_states;
	int sink;
	long max_exeps;
	char *statuces;
	int len;
	int ns = M->ns;
	int *indices;
	char* lambda = bintostr(c, var);
	boolean extraBitNeeded = FALSE;


	sink = find_sink(M);
	assert(sink>-1);
	//printf("\n\n SINK %d\n\n\n", sink);

	char* symbol = (char *) malloc((var + 2) * sizeof(char));//var+2 since we may need extra bit
	//construct the added paths for the initial state
	state_paths = pp = make_paths(M->bddm, M->q[M->s]);
	//printf("\n\n INIT %d \n\n", M1->s);

	while (pp) {
		if (pp->to != sink) {
			for (j = 0; j < var; j++) {
				//the following for loop can be avoided if the indices are in order
				for (tp = pp->trace; tp && (tp->index != oldIndices[j]); tp
						= tp->next)
					;
				if (tp) {
					if (tp->value)
						symbol[j] = '1';
					else
						symbol[j] = '0';
				} else
					symbol[j] = 'X';
			}
			symbol[j] = '\0';
			if (isIncludeLambda(symbol, lambda, var)){
				if (pp->to == M->q[M->s]){
					result = dfaCopy(M);
					kill_paths(state_paths);
					free(symbol);
					free(lambda);
					return result;
				}
				else{
					extraBitNeeded = TRUE;
					break;
				}
			}
		}
		pp = pp->next;
	}
	kill_paths(state_paths);

	len = extraBitNeeded? var + 1: var;
	indices = extraBitNeeded? allocateArbitraryIndex(len) : oldIndices;
	max_exeps = 1 << len; //maybe exponential
	dfaSetup(ns, len, indices);
	exeps = (char *) malloc(max_exeps * (len + 1) * sizeof(char));
	to_states = (int *) malloc(max_exeps * sizeof(int));
	statuces = (char *) malloc((ns + 1) * sizeof(char));


	for (i = 0; i < M->ns; i++) {
		//construct the added paths for the initial state
		state_paths = pp = make_paths(M->bddm, M->q[i]);
		//printf("\n\n INIT %d \n\n", M1->s);

		k = 0;
		while (pp) {
			if (pp->to != sink) {
				to_states[k] = pp->to;
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp
							= tp->next)
						;
					if (tp) {
						if (tp->value)
							exeps[k * (len + 1) + j] = '1';
						else
							exeps[k * (len + 1) + j] = '0';
					} else
						exeps[k * (len + 1) + j] = 'X';
				}
				exeps[k * (len + 1) + j] = '0';// for init state extrabit 1 goes to self loop for lambda. Everthing else goes to sink
				exeps[k * (len + 1) + len] = '\0';// if no extrabit will overwrite assign in prev line
				k++;
			}
			pp = pp->next;
		}
		kill_paths(state_paths);

		if (i == M->s){
			strcpy(symbol, lambda);
			if (extraBitNeeded){
				symbol[var] = '1';
				symbol[len] = '\0';
			}
			dfaAllocExceptions(k+1);
			dfaStoreException(M->s, symbol);
		}
		else
			dfaAllocExceptions(k);
		for (k--; k >= 0; k--)
			dfaStoreException(to_states[k], exeps + k * (len + 1));
		dfaStoreState(sink);

		if (M->f[i] == -1)
			statuces[i] = '-';
		else if (M->f[i] == 1)
			statuces[i] = '+';
		else
			statuces[i] = '0';
	}

	statuces[ns] = '\0';
	DFA* tmpM = dfaBuild(statuces);


	free(exeps);
	free(symbol);
	free(to_states);
	free(statuces);
	free(lambda);


	if (extraBitNeeded){
		free(indices);
		result = dfaProject(tmpM, ((unsigned)var));
		dfaFree(tmpM); tmpM = NULL;
		tmpM = dfaMinimize(result);
		dfaFree(result);result = NULL;
		return tmpM;
	}
	else
	{
		result = dfaMinimize(tmpM);
		dfaFree(tmpM);tmpM = NULL;
		return result;
	}
}

struct transition_list_type *getTransitionRelationMatrix(DFA* M, char *lambda,
		int var, int* indices) {
	paths state_paths, pp;
	trace_descr tp;
	int j;
	int sink = find_sink(M);
	char *symbol = (char *) malloc((var+1)*sizeof(char));
	struct transition_list_type *finallist = NULL;
	int i;
	for (i = 0; i < M->ns; i++) {

		state_paths = pp = make_paths(M->bddm, M->q[i]);

		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
							tp->next)
						;

					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[j] = '\0';
				if (isIncludeLambda(symbol, lambda, var)) {
					finallist = transition_enqueue(finallist, i, pp->to);
				}

			}
			pp = pp->next;
		} //end while

		kill_paths(state_paths);
	}

//	printf("list of states reachable on \\s:");
//	transition_print_ilt(finallist);
//	printf("\n");

	free(symbol);
	return finallist;
}

struct int_list_type * findReversePathsOnLambda(struct transition_list_type *transition_relation, struct int_list_type *finallist, int current_state){
	finallist = enqueue(finallist, current_state);
	struct transition_type *tmp2;
	// for all transition relation on lambda
	for (tmp2 = transition_relation->head; tmp2 != NULL; tmp2 = tmp2->next) {
		// if current transition has current state as destination then follow transition in reverse if has not been followed before
		if ((current_state == tmp2->to) && (!check_value(finallist, tmp2->from))){
			finallist = findReversePathsOnLambda(transition_relation, finallist, tmp2->from);
		}
	}
	return finallist;

}

struct int_list_type *states_reach_accept_lambda(DFA* M, char* lambda, int var, int* indices){

	struct transition_list_type *transition_relation = getTransitionRelationMatrix(M, lambda, var, indices);
	if (transition_relation == NULL)
		return NULL;
	struct int_list_type *finallist=NULL;

	// for each accepting state get list that reach the accepting state on lambda

	struct transition_type *tmp = transition_relation->head;
	while (tmp != NULL) {
		// for each accept state
		if (M->f[tmp->to] == 1){
			// if accept state has not been processed before
			if (!check_value(finallist, tmp->to)){
				finallist = findReversePathsOnLambda(transition_relation, finallist, tmp->to);
			}
		}
		tmp = tmp->next;
	}


	// free unneeded memory
	transition_free_ilt(transition_relation);
//	printf("states that reach an accepting state on lambda: ");
//	print_ilt(finallist);
//	printf("\n");
	return finallist;
}

DFA *dfaRightTrim(DFA *M, char c, int var, int *oldindices) {
	DFA *result = NULL;
	DFA *tmpM = NULL;
	char* lambda = bintostr(c, var);
	int aux = 1;
	struct int_list_type *states = NULL;

	int maxCount = 0;

	int *indices = oldindices; //indices is updated if you need to add auxiliary bits

	paths state_paths, pp;
	trace_descr tp;

	int i, j, z, k;

	char *exeps;
	int *to_states;
	long max_exeps;
	char *statuces;
	int len = var;
	int sink, new_accept_state;
	boolean split_char;

	int numOfChars;
	char** charachters;
	int size;

	char *symbol;


	states = states_reach_accept_lambda(M, lambda, var, indices);
	if (states == NULL ){
		free(lambda);
		return dfaCopy(M);
	}

	symbol = (char *) malloc((var + 1) * sizeof(char));
	maxCount = states->count;

	if (maxCount > 0) { //Need auxiliary bits when there exist some outgoing edges
		aux = 1;
		len = var + aux; // extra aux bits
		indices = allocateArbitraryIndex(len);
	}

	max_exeps = 1 << len; //maybe exponential
	sink = find_sink(M);
	assert(sink > -1);
	new_accept_state = M->ns;

	numOfChars = 1 << var;
	charachters = (char**) malloc(numOfChars * (sizeof(char*)));

	//pairs[i] is the list of all reachable states by \sharp1 \bar \sharp0 from i

	dfaSetup(M->ns + 1, len, indices); //add one new accept state
	exeps = (char *) malloc(max_exeps * (len + 1) * sizeof(char)); //plus 1 for \0 end of the string
	to_states = (int *) malloc(max_exeps * sizeof(int));
	statuces = (char *) malloc((M->ns + 2) * sizeof(char)); //plus 2, one for the new accept state and one for \0 end of the string

	// for each original state
	for (i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);
		k = 0;
		// for each transition out from current state (state i)
		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
							tp->next)
						;

					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[var] = '\0';
				// first copy to original destination without removing lambda
				to_states[k] = pp->to;
				for (j = 0; j < var; j++)
					exeps[k * (len + 1) + j] = symbol[j];
				exeps[k * (len + 1) + var] = '0';
				exeps[k * (len + 1) + len] = '\0';
				k++;

				split_char = check_value(states, pp->to);
				if (split_char == TRUE) {
					// second copy to new accept state after removing lambda
					if (!isIncludeLambda(symbol, lambda, var)) {
						// no lambda send as it is
						to_states[k] = new_accept_state; // destination new accept state
						for (j = 0; j < var; j++)
							exeps[k * (len + 1) + j] = symbol[j];
						exeps[k * (len + 1) + var] = '1';
						exeps[k * (len + 1) + len] = '\0';
						k++;
					} else {
						// remove lambda then send copy to new accept state
						removeTransitionOnChar(symbol, lambda, var, charachters,
								&size);
						for (z = 0; z < size; z++) {
							// first copy of non-bamda char to original destination
//							printf("%s, ", charachters[z]);
							to_states[k] = new_accept_state; // destination new accept state
							for (j = 0; j < var; j++)
								exeps[k * (len + 1) + j] = charachters[z][j];
							exeps[k * (len + 1) + var] = '1';
							exeps[k * (len + 1) + len] = '\0';
							k++;
							free(charachters[z]);
						}
//						printf("\n");
					}
				}
			}
			pp = pp->next;
		} //end while

		dfaAllocExceptions(k);
		for (k--; k >= 0; k--)
			dfaStoreException(to_states[k], exeps + k * (len + 1));
		dfaStoreState(sink);
		statuces[i] = '-';

		kill_paths(state_paths);
	} // end for each original state

	// add new accept state
	dfaAllocExceptions(0);
	dfaStoreState(sink);
	statuces[new_accept_state] = '+';

	statuces[M->ns + 1] = '\0';
	result = dfaBuild(statuces);
//	printf("dfaAfterRightTrimBeforeMinimize\n");
//	dfaPrintGraphviz(result, len, indices);

	j = len - 1;
	tmpM = dfaProject(result, (unsigned) j);
	dfaFree(result);result = NULL;
	result = dfaMinimize(tmpM);
	dfaFree(tmpM);tmpM = NULL;

	// if original accept epsilon or start state reaches an accept state on lambda (\s+ is an element of L(M))
    //then add epsilon to language
	if (M->f[M->s] == 1 || check_value(states, M->s)){
		tmpM = dfa_union_empty_M(result, var, indices);
		dfaFree(result); result = NULL;
		result = tmpM;
	}
	free(exeps);
	//printf("FREE ToState\n");
	free(to_states);
	//printf("FREE STATUCES\n");
	free(statuces);
	free(charachters);

	free_ilt(states);
	free(lambda);

    free(symbol);
    if (maxCount > 0) free(indices);
    
	return result;

}




/**
 * Muath documentation:
 * returns a list of states containing each state s that has at least one transition on lambda
 * into it and one transition on non-lambda out of it (except for sink state which is ignored)
 * end Muath documentation
 */
struct int_list_type *reachable_states_lambda_in_nout1(DFA *M, char *lambda, int var, int* indices){

	paths state_paths, pp;
	trace_descr tp;
	int j, current;
	int sink = find_sink(M);
	char *symbol;
	struct int_list_type *finallist=NULL;
	if(_FANG_DFA_DEBUG)dfaPrintVerbose(M);
	symbol=(char *)malloc((var+1)*sizeof(char));
	current = 0;
	boolean keeploop = TRUE;
	while (keeploop){
		keeploop = FALSE;
		state_paths = pp = make_paths(M->bddm, M->q[current]);
		while (pp && (!keeploop)) {
			if(pp->to != sink){
				// construct transition from current to pp->to and store it in symbol
				for (j = 0; j < var; j++) {
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);
					if (tp) {
						if (tp->value) symbol[j]='1';
						else symbol[j]='0';
					}
					else
						symbol[j]='X';

				}
				symbol[j]='\0';
				// if transition from current state to pp->to state is on labmda
				if(isIncludeLambda(symbol, lambda, var)){
					//if not added before
					if (!check_value(finallist, pp->to)){
						if (current == 0)
							finallist = enqueue(finallist, current);
						finallist = enqueue(finallist, pp->to);
						current = pp->to;
						keeploop = TRUE;
					}
				}
			}
			pp = pp->next;
		}
        kill_paths(state_paths);
	}
	if (finallist!=NULL){
//		printf("list of states reachable on \\s:");
//		print_ilt(finallist);
//		printf("\n");
	}
	free(symbol);
	return finallist;
}

void removeTransitionOnCharHelper(char** result, char* transitions, char* charachter, int* indexInResult, int currentBit, int var){
	int i;
	if (transitions[currentBit] == 'X')
	{
		transitions[currentBit] = '0';
		removeTransitionOnCharHelper(result, transitions, charachter, indexInResult, currentBit, var);
		transitions[currentBit] = '1';
		removeTransitionOnCharHelper(result, transitions, charachter, indexInResult, currentBit, var);
		transitions[currentBit] = 'X';

	}

	else if (transitions[currentBit] != charachter[currentBit])
	{
		result[*indexInResult] = (char*) malloc((var+1)*(sizeof (char)));
		for (i = 0; i < var; i++)
			result[*indexInResult][i] = transitions[i];
		result[*indexInResult][var] = '\0';
		(*indexInResult)++;
	}

	else {
			if (currentBit < (var - 1))
				removeTransitionOnCharHelper(result, transitions, charachter, indexInResult, currentBit + 1, var);
	}

}

void removeTransitionOnChar(char* transitions, char* charachter, int var, char** result, int* pSize){
	int indexInResult = 0;
	removeTransitionOnCharHelper(result, transitions, charachter, &indexInResult, 0, var);
	*pSize = indexInResult;
}


DFA *dfaLeftTrim(DFA *M, char c, int var, int *oldindices)
{
  DFA *result = NULL;
  DFA *tmpM = NULL;
  char* lambda = bintostr(c, var);
  int aux=0;
  struct int_list_type *states=NULL;
  struct int_type *tmpState=NULL;

  int maxCount = 0;

  int *indices = oldindices; //indices is updated if you need to add auxiliary bits

  paths state_paths, pp;
  trace_descr tp;

  int i, j, z, k;

  char *exeps;
  int *to_states;
  long max_exeps;
  char *statuces;
  int len=var;
  int sink;

  char *auxbit=NULL;

  int numOfChars;
  char** charachters;
  int size;

  char *symbol;


  states = reachable_states_lambda_in_nout1(M, lambda, var, indices);
  if(states == NULL){
	  free(lambda);
	  return dfaCopy(M);
  }

  symbol=(char *)malloc((var+1)*sizeof(char));

  maxCount = states->count;

  if(maxCount>0){ //Need auxiliary bits when there exist some outgoing edges
    aux = get_hsig(maxCount);
    if(_FANG_DFA_DEBUG) printf("\n There are %d reachable states, need to add %d auxiliary bits\n", maxCount, aux);
    auxbit = (char *) malloc(aux*sizeof(char));
    len = var+aux; // extra aux bits
    indices = allocateArbitraryIndex(len);
  }



  max_exeps=1<<len; //maybe exponential
  sink=find_sink(M);
  assert(sink >-1);

  numOfChars = 1<<var;
  charachters = (char**) malloc(numOfChars * (sizeof (char*)));

  //pairs[i] is the list of all reachable states by \sharp1 \bar \sharp0 from i


  dfaSetup(M->ns+1, len, indices); //add one new initial state
  exeps=(char *)malloc(max_exeps*(len+1)*sizeof(char)); //plus 1 for \0 end of the string
  to_states=(int *)malloc(max_exeps*sizeof(int));
  statuces=(char *)malloc((M->ns+2)*sizeof(char));

  //printf("Before Replace Char\n");
  //dfaPrintVerbose(M);

  k=0;
  //setup for the initial state
  tmpState = states->head;
	for (z = 1; z <= states->count; z++) {
		state_paths = pp = make_paths(M->bddm, M->q[tmpState->value]);
		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp = tp->next);

					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[j] = '\0';

				if (!isIncludeLambda(symbol, lambda, var)) { // Only Consider Non-lambda case
					to_states[k] = pp->to + 1;
					for (j = 0; j < var; j++)
						exeps[k * (len + 1) + j] = symbol[j];

					set_bitvalue(auxbit, aux, z); // aux = 3, z=4, auxbit 001
					for (j = var; j < len; j++) { //set to xxxxxxxx100
						exeps[k * (len + 1) + j] = auxbit[len - j - 1];
					}
					exeps[k * (len + 1) + len] = '\0';
					k++;
				}
				else {
					removeTransitionOnChar(symbol, lambda, var, charachters, &size);
					for (i = 0; i < size; i++)
					{
//						printf("%s, ", charachters[i]);
						to_states[k] = pp->to + 1;
						for (j = 0; j < var; j++)
							exeps[k * (len + 1) + j] = charachters[i][j];

						set_bitvalue(auxbit, aux, z); // aux = 3, z=4, auxbit 001
						for (j = var; j < len; j++) { //set to xxxxxxxx100
							exeps[k * (len + 1) + j] = auxbit[len - j - 1];
						}
						exeps[k * (len + 1) + len] = '\0';
						free(charachters[i]);
						k++;
					}
//					printf("\n");
				}
			} //end if
			pp = pp->next;
		} //end while
		kill_paths(state_paths);
		tmpState = tmpState->next;
	} //end for

  dfaAllocExceptions(k);
  for(k--;k>=0;k--)
    dfaStoreException(to_states[k],exeps+k*(len+1));
  dfaStoreState(sink+1);

  // if we can reach an accept state on lambda* then
  // accept epsilon (start state becomes an accepting state)
  if(check_accept(M, states))	statuces[0]='+';
  else 	statuces[0]='-';



  //for the rest of states (shift one state)
	for (i = 0; i < M->ns; i++) {

		state_paths = pp = make_paths(M->bddm, M->q[i]);
		k = 0;

		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
							tp->next)
						;

					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}

					to_states[k] = pp->to + 1;
					for (j = 0; j < var; j++)
						exeps[k * (len + 1) + j] = symbol[j];
					for (j = var; j < len; j++) { //set to xxxxxxxx100
						exeps[k * (len + 1) + j] = '0';
					}
					exeps[k * (len + 1) + len] = '\0';
					k++;

			}
			pp = pp->next;
		} //end while

		dfaAllocExceptions(k);
		for (k--; k >= 0; k--)
			dfaStoreException(to_states[k], exeps + k * (len + 1));
		dfaStoreState(sink + 1);

		if (M->f[i] == 1)
			statuces[i + 1] = '+';
		else if (i == sink)
			statuces[i + 1] = '-';
		else
			statuces[i + 1] = '0';
		kill_paths(state_paths);
	}

	statuces[M->ns + 1] = '\0';
	result = dfaBuild(statuces);
//	printf("dfaAfterleftTrimBeforeMinimize\n");
//	dfaPrintGraphviz(result, len, indices);
	//	dfaPrintVerbose(result);
	for (i = 0; i < aux; i++) {
		j = len - i - 1;
		tmpM = dfaProject(result, (unsigned) j);
		dfaFree(result); result = NULL;
		result = dfaMinimize(tmpM);
		dfaFree(tmpM); tmpM = NULL;
		//		printf("\n After projecting away %d bits", j);
		//		dfaPrintVerbose(result);
	}
  free(exeps);
  //printf("FREE ToState\n");
  free(to_states);
  //printf("FREE STATUCES\n");
  free(statuces);
  free(charachters);
  free(lambda);
    free(symbol);

    if(maxCount>0) {
        free(indices);
        free(auxbit);
    }
  free_ilt(states);

  return result;

}

// Stores the trimmed input string into the given output buffer, which must be
// large enough to store the result.  If it is too small, the output is
// truncated.
size_t trimwhitespace(char *out, size_t len, const char *str)
{
  if(len == 0)
    return 0;

  const char *end;
  size_t out_size;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
  {
    *out = 0;
    return 1;
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  end++;

  // Set output size to minimum of trimmed string length and buffer size minus 1
  out_size = (end - str) < len-1 ? (end - str) : len-1;

  // Copy trimmed string and add null terminator
  memcpy(out, str, out_size);
  out[out_size] = 0;

  return out_size;
}

DFA* dfaTrim(DFA* inputAuto, char c, int var, int* indices){
	DFA *leftTrimmed, *leftThenRightTrimmed;
	leftTrimmed = dfaLeftTrim(inputAuto, c, var, indices);
//	printf("\n\n\ndfa after left trim\n");
//	dfaPrintGraphvizAsciiRange(leftTrimmed, var, indices, 0);
	leftThenRightTrimmed = dfaRightTrim(leftTrimmed, c, var, indices);
	dfaFree(leftTrimmed);
	return leftThenRightTrimmed;
}

/**
 * trims a list of characters stored in array chars[].
 * num is the size of array chars
 */
DFA* dfaTrimSet(DFA* inputAuto, char chars[], int num, int var, int* indices){
	DFA *leftTrimmed, *leftThenRightTrimmed, *tmp;
	int i;
	assert(chars != NULL);

	tmp = dfaCopy(inputAuto);

	for (i = 0; i < num; i++){
		leftTrimmed = dfaLeftTrim(tmp, chars[i], var, indices);
		dfaFree(tmp);
		tmp = leftTrimmed;
	}

	for (i = 0; i < num; i++){
		leftThenRightTrimmed = dfaRightTrim(tmp, chars[i], var, indices);
		dfaFree(tmp);
		tmp = leftThenRightTrimmed;
	}

	return leftThenRightTrimmed;
}

DFA* dfaPreTrim(DFA* inputAuto, char c, int var, int* indices){
	DFA *rightPreTrimmed, *rightThenLeftPreTrimmed;
	rightPreTrimmed = dfaPreRightTrim(inputAuto, c, var, indices);
//	printf("\n\n\ndfa after pre right trim\n");
//	dfaPrintGraphvizAsciiRange(rightPreTrimmed, var, indices, 0);
	rightThenLeftPreTrimmed = dfaPreLeftTrim(rightPreTrimmed, c, var, indices);
	dfaFree(rightPreTrimmed);
	return rightThenLeftPreTrimmed;
}

/**
 * pretrims a list of characters stored in array chars[].
 * num is the size of array chars
 */
DFA* dfaPreTrimSet(DFA* inputAuto, char chars[], int num, int var, int* indices){
	DFA *leftPreTrimmed, *leftThenRightPreTrimmed, *tmp;
	int i;
	assert(chars != NULL);

	tmp = dfaCopy(inputAuto);

	for (i = 0; i < num; i++){
		leftPreTrimmed = dfaPreLeftTrim(tmp, chars[i], var, indices);
		dfaFree(tmp);
		tmp = leftPreTrimmed;
	}

	for (i = 0; i < num; i++){
		leftThenRightPreTrimmed = dfaPreRightTrim(tmp, chars[i], var, indices);
		dfaFree(tmp);
		tmp = leftThenRightPreTrimmed;
	}

	return leftThenRightPreTrimmed;
}

DFA* dfaAddSlashes(DFA* inputAuto, int var, int* indices){
//    if (isLengthFiniteDFS(inputAuto, var, indices)){
        DFA* retMe1 = dfa_escape_single_finite_lang(inputAuto, var, indices, '\\', '\\');
        // escape single quota '
        // ' -> \'
//        DFA* retMe2 = dfa_escape_single_finite_lang(retMe1, var, indices, '\'', '\\');
//        dfaFree(retMe1);
//        // escape single quota '
//        // " -> \"
//        retMe1 = dfa_escape_single_finite_lang(retMe2, var, indices, '"', '\\');
//        dfaFree(retMe2);
        return retMe1;
//    }
//    else
//    {
//        DFA* searchAuto = dfa_construct_string("\\", var, indices);
//        char* replaceStr = "\\\\";
//        DFA* retMe1 = dfa_replace_extrabit(inputAuto, searchAuto, replaceStr, var, indices);
//        dfaFree(searchAuto); searchAuto = NULL;
//        printf("passed first replace in addSlashes\n");
//        // escape single quota '
//        // ' -> \'
//        searchAuto = dfa_construct_string("'", var, indices);
//        replaceStr = "\\'";
//        DFA* retMe2 = dfa_replace_extrabit(retMe1, searchAuto, replaceStr, var, indices);
//        dfaFree(searchAuto); searchAuto = NULL;
//        dfaFree(retMe1); retMe1 = NULL;
//        printf("passed second replace  in addSlashes\n");
//        // escape double quota "
//        // " -> \"
//        searchAuto = dfa_construct_string("\"", var, indices);
//        replaceStr = "\\\"";
//        retMe1 = dfa_replace_extrabit(retMe2, searchAuto, replaceStr, var, indices);
//        dfaFree(searchAuto); searchAuto = NULL;
//        dfaFree(retMe2); retMe2 = NULL;
//        printf("passed third replace in addSlashes\n");
//        return retMe1;
//    }
}

DFA* dfaPreAddSlashes(DFA* inputAuto, int var, int* indices){

//    if (isLengthFiniteDFS(inputAuto, var, indices)){
//        DFA* retMe1 = dfa_pre_escape_single_finite_lang(inputAuto, var, indices, '"', '\\');
    DFA* retMe1 = dfa_pre_escape_single_finite_lang(inputAuto, var, indices, '\\', '\\');
//        // escape single quota '
//        // ' -> \'
//        DFA* retMe2 = dfa_pre_escape_single_finite_lang(retMe1, var, indices, '\'', '\\');
//        dfaFree(retMe1);
        // escape single quota '
        // " -> \"
//        retMe1 = dfa_pre_escape_single_finite_lang(retMe2, var, indices, '\\', '\\');
//        dfaFree(retMe2);
        return retMe1;
//    }
//    else
//    {
//
//	// pre escape backslash \
//	// \ -> \\
//
//	DFA* searchAuto = dfa_construct_string("\\", var, indices);
//		char* replaceStr = "\\\\";
//		DFA* retMe1 = dfa_pre_replace_str(inputAuto, searchAuto, replaceStr, var, indices);
//		dfaFree(searchAuto); searchAuto = NULL;
//		// escape single quota '
//		// ' -> \'
//		searchAuto = dfa_construct_string("'", var, indices);
//		replaceStr = "\\'";
//		DFA* retMe2 = dfa_pre_replace_str(retMe1, searchAuto, replaceStr, var, indices);
//		dfaFree(searchAuto); searchAuto = NULL;
//		dfaFree(retMe1); retMe1 = NULL;
//
//		// escape double quota "
//		// " -> \"
//		searchAuto = dfa_construct_string("\"", var, indices);
//		replaceStr = "\\\"";
//		retMe1 = dfa_pre_replace_str(retMe2, searchAuto, replaceStr, var, indices);
//		dfaFree(searchAuto); searchAuto = NULL;
//		dfaFree(retMe2); retMe2 = NULL;
//
//		return retMe1;
//    }
}

void copyPrevBits(char* to, char* from, int limit){
	int i;
	//copy previous bits
	for (i = 0; i < limit; i++)
		to[i] = from[i];
}

/**
 */
void getUpperCaseCharsHelper(char** result, char* transitions, int* indexInResult, int currentBit, int var, char* prev){
	int i;
	if (transitions[currentBit] == 'X')
	{
		transitions[currentBit] = '0';
		getUpperCaseCharsHelper(result, transitions, indexInResult, currentBit, var, prev);
		transitions[currentBit] = '1';
		getUpperCaseCharsHelper(result, transitions, indexInResult, currentBit, var, prev);
		transitions[currentBit] = 'X';
	}
	// things that do not contain upper case
	else if ( (transitions[0] == '1' && currentBit == 0) ||
			  (transitions[1] == '0' && currentBit == 1) ||
			  (transitions[2] == '0' && currentBit == 2) ||
			  (transitions[5] == '1' && transitions[3] == '1' && currentBit == 5) ||
			  (transitions[7] ==  transitions[3] && currentBit == 7)
			 )
	{
		result[*indexInResult] = (char*) malloc((var+2)*(sizeof (char)));
		for (i = 0; i < var; i++){
			result[*indexInResult][i] = transitions[i];
		}
		result[*indexInResult][var] = '0';//extrabit
		result[*indexInResult][var+1] = '\0';
		(*indexInResult)++;
	}
	else if ( (transitions[3] != transitions[4] && (currentBit == 4 )) ||
			  (transitions[3] != transitions[6] && currentBit == 6 ) ||
			  (transitions[3] != transitions[7] && currentBit == 7 ) ||
			  (transitions[5] == '1' && transitions[3] ==  '0' && currentBit == 5)
				 ){
		result[*indexInResult] = (char*) malloc((var+2)*(sizeof(char)));
		for (i = 0; i < var; i++)
			result[*indexInResult][i] = transitions[i];
		// only difference between capital and small is bit number 2
		result[*indexInResult][2] = '0';
		// extrabit should be 1 since we may already have same small letter originally with 0
		result[*indexInResult][var] = '1';//extrabit
		result[*indexInResult][var+1] = '\0';
		(*indexInResult)++;
	}
	else{
		if (currentBit < (var-1)){
			getUpperCaseCharsHelper(result, transitions, indexInResult, currentBit + 1, var, prev);
		}
		else
			assert(FALSE);
	}

}
/**
 */
void getLowerUpperCaseCharsPrePostHelper(char** result, char* transitions, int* indexInResult, int currentBit, int var, char* prev, boolean lowerCase, boolean preImage){
	int i;
	if (transitions[currentBit] == 'X')
	{
		transitions[currentBit] = '0';
		getLowerUpperCaseCharsPrePostHelper(result, transitions, indexInResult, currentBit, var, prev, lowerCase, preImage);
		transitions[currentBit] = '1';
		getLowerUpperCaseCharsPrePostHelper(result, transitions, indexInResult, currentBit, var, prev, lowerCase, preImage);
		transitions[currentBit] = 'X';
	}
	// things that do not contain lower case
	else if ( (transitions[0] == '1' && currentBit == 0) ||
			  (transitions[1] == '0' && currentBit == 1) ||
			  (currentBit == 2 && ((transitions[2] == '1' && lowerCase) || (transitions[2] == '0' && !lowerCase))) ||
			  (transitions[5] == '1' && transitions[3] == '1' && currentBit == 5) ||
			  (transitions[7] ==  transitions[3] && currentBit == 7)
			 )
	{
		result[*indexInResult] = (char*) malloc((var+2)*(sizeof (char)));
		for (i = 0; i < var; i++){
			result[*indexInResult][i] = transitions[i];
		}
		result[*indexInResult][var] = '0';//extrabit
		result[*indexInResult][var+1] = '\0';
		(*indexInResult)++;
	}
	else if ( (transitions[3] != transitions[4] && (currentBit == 4 )) ||
			  (transitions[3] != transitions[6] && currentBit == 6 ) ||
			  (transitions[3] != transitions[7] && currentBit == 7 ) ||
			  (transitions[5] == '1' && transitions[3] ==  '0' && currentBit == 5)
				 ){
		result[*indexInResult] = (char*) malloc((var+2)*(sizeof(char)));
		for (i = 0; i < var; i++)
			result[*indexInResult][i] = transitions[i];
		// only difference between capital and small is bit number 2
		if (!preImage){
			if (lowerCase)
				result[*indexInResult][2] = '1';
			else
				result[*indexInResult][2] = '0';
        }
		// extrabit should be 1 since we may already have same small letter originally with 0
		result[*indexInResult][var] = '1';//extrabit
		result[*indexInResult][var+1] = '\0';
		(*indexInResult)++;
		if (preImage){
			result[*indexInResult] = (char*) malloc((var+2)*(sizeof(char)));
			for (i = 0; i < var; i++)
				result[*indexInResult][i] = transitions[i];
			// only difference between capital and small is bit number 2
			if (lowerCase)
				result[*indexInResult][2] = '1';
			else
				result[*indexInResult][2] = '0';
			// extrabit should be 1 since we may already have same small letter originally with 0
			result[*indexInResult][var] = '1';//extrabit
			result[*indexInResult][var+1] = '\0';
			(*indexInResult)++;
		}
	}
	else{
			if (currentBit < (var-1)){
				getLowerUpperCaseCharsPrePostHelper(result, transitions, indexInResult, currentBit + 1, var, prev, lowerCase, preImage);
			}
			else
				assert(FALSE);
	}

}

/**
 * returns same chars in transitions unless char is capital in which case it is converted into small
 * or visa versa
 * examples:
 * A=01000001 -> a=[011000011]
 * {@, A, B, C ,D, E, F, G}=01000XXX -> {@, a, b, c, d, e, f, g}=[010000000, 011000011, 0110001X1, 011001XX1]
 * extrabit will be used. 0 for original letters, 1 for new small letters (converted from capital) to differentiate from original small chars
 */
void getLowerUpperCaseCharsPrePost(char* transitions, int var, char** result, int* pSize, boolean lowerCase, boolean preImage){
	int indexInResult = 0;
	char* prev = (char*) malloc(var*(sizeof(char)));
	getLowerUpperCaseCharsPrePostHelper(result, transitions, &indexInResult, 0, var, prev, lowerCase, preImage);
	*pSize = indexInResult;
}

/**
 * This functions models any function that changes all capital letters in a string to small ones (for example strtolower in php)
 * M: dfa to process
 * var: number of bits per character(for ASCII it is 8 bits)
 * indices: the indices
 * output: return D such that L(D) = { W_1S_1W_2S2..W_nS_n | W_1C_1W_2C2..W_nC_n element_of L(M) && W_i element_of Sigma* && S_i, C_i element_of Sigma && S_i = LowerCase(C_i)}
 */
DFA* dfaPrePostToLowerUpperCaseHelper(DFA* M, int var, int* oldIndices, boolean lowerCase, boolean preImage){
		DFA *result;
		paths state_paths, pp;
		trace_descr tp;
		int i, j, n, k;
		char *exeps;
		int *to_states;
		int sink;
		long max_exeps;
		char *statuces;
		int len;
		int ns = M->ns;

		len = var + 1;
		int* indices = allocateArbitraryIndex(len);

		max_exeps = 1 << len; //maybe exponential
		sink = find_sink(M);
		assert(sink>-1);

		char* symbol = (char *) malloc((len + 1) * sizeof(char));//len+1 since we need extra bit
		exeps = (char *) malloc(max_exeps * (len + 1) * sizeof(char));
		to_states = (int *) malloc(max_exeps * sizeof(int));
		statuces = (char *) malloc((ns + 1) * sizeof(char));
		int numOfChars = 1 << len;
		char** charachters = (char**) malloc(numOfChars * (sizeof (char*)));
		int size = 0;


		dfaSetup(ns, len, indices);
		for (i = 0; i < M->ns; i++) {
			state_paths = pp = make_paths(M->bddm, M->q[i]);
			k = 0;
			while (pp) {
				if (pp->to != sink) {
					for (j = 0; j < var; j++) {
						//the following for loop can be avoided if the indices are in order
						for (tp = pp->trace; tp && (tp->index != indices[j]); tp
								= tp->next)
							;
						if (tp) {
							if (tp->value)
								symbol[j] = '1';
							else
								symbol[j] = '0';
						} else
							symbol[j] = 'X';
					}
					symbol[var] = '\0';
					// convert symbol into a list of chars where we replace each capital letter with small letter
					getLowerUpperCaseCharsPrePost(symbol, var, charachters, &size, lowerCase, preImage);
					for (n = 0; n < size; n++)
					{
//						printf("%s, ", charachters[n]);
						to_states[k] = pp->to;
						for (j = 0; j < len; j++)
							exeps[k * (len + 1) + j] = charachters[n][j];
						exeps[k * (len + 1) + len] = '\0';
						free(charachters[n]);
						k++;
					}
//					printf("\n");
				}
				pp = pp->next;
			}
			kill_paths(state_paths);

			// if accept state create a self loop on lambda
			dfaAllocExceptions(k);
			for (k--; k >= 0; k--)
				dfaStoreException(to_states[k], exeps + k * (len + 1));
			dfaStoreState(sink);

			if (M->f[i] == -1)
				statuces[i] = '-';
			else if (M->f[i] == 1)
				statuces[i] = '+';
			else
				statuces[i] = '0';
		}

		statuces[ns] = '\0';
		DFA* tmpM = dfaBuild(statuces);
//		dfaPrintGraphviz(tmpM, len, indices);
//		dfaPrintVerbose(tmpM);
		flush_output();
		result = dfaProject(tmpM, ((unsigned)var));
		dfaFree(tmpM); tmpM = NULL;
		tmpM = dfaMinimize(result);
		dfaFree(result);result = NULL;

		free(exeps);
		free(symbol);
		free(to_states);
		free(statuces);
		free(indices);
		free(charachters);

		return tmpM;
}

DFA* dfaToLowerCase(DFA* M, int var, int* indices){
	return dfaPrePostToLowerUpperCaseHelper(M, var, indices, TRUE, FALSE);
}

DFA* dfaToUpperCase(DFA* M, int var, int* indices){
	return dfaPrePostToLowerUpperCaseHelper(M, var, indices, FALSE, FALSE);
}

DFA* dfaPreToLowerCase(DFA* M, int var, int* indices){
	return dfaPrePostToLowerUpperCaseHelper(M, var, indices, FALSE, TRUE);
}

DFA* dfaPreToUpperCase(DFA* M, int var, int* indices){
	return dfaPrePostToLowerUpperCaseHelper(M, var, indices, FALSE, TRUE);
}



int getNumberOfNewStates(DFA *M, int var, int *indices, char *lambda, int sink){
    int i, j, num = 0;
    paths state_paths, pp;
	trace_descr tp;
    char *symbol = (char *) malloc((var + 1) * sizeof(char));

    // for each original state
	for (i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);
		// for each transition out from current state (state i)
		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
                         tp->next)
						;
                    
					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[var] = '\0';
                
                
                // second copy to new accept state after removing lambda
                if (isIncludeLambda(symbol, lambda, var)) {
//                    printf("labmda founded from %d -> %d\n", i, pp->to);
                    num++;
                } 
			}
			pp = pp->next;
		} //end while        
		kill_paths(state_paths);
	} // end for each original s
    
    free(symbol);
    
    return num;
}

DFA *dfa_escape_single_finite_lang(DFA *M, int var, int *oldindices, char c, char escapeChar){
    DFA *result = NULL;
	char* lambda = bintostr(c, var);
  	char* escapeLambda = bintostr(escapeChar, var);
   	char* lambdaExtraBit = (char *) malloc(sizeof(char) * (strlen(lambda) + 2));
	struct int_list_type *states = NULL;
    
	int maxCount = 0;
        
	paths state_paths, pp;
	trace_descr tp;
    
	int i, j, z, k;
    
	char *exeps;
	int *to_states;
	long max_exeps;
	char *statuces;
	int len = var + 1;
	int sink, new_state_counter;
    int *destinations;
    
	int numOfChars;
	char** charachters;
	int size;
    
	char *symbol;
    
    
//	states = states_reach_accept_lambda(M, lambda, var, indices);
//	if (states == NULL ){
//		free(lambda);
//		return dfaCopy(M);
//	}
    
	symbol = (char *) malloc((var + 1) * sizeof(char));
	maxCount = 0;
    
    int *indices = allocateArbitraryIndex(len);
	max_exeps = 1 << len; //maybe exponential
	sink = find_sink(M);
	assert(sink > -1);
    
	numOfChars = 1 << var;
	charachters = (char**) malloc(numOfChars * (sizeof(char*)));
    
    int num_new_states = getNumberOfNewStates(M, var, indices, lambda, sink);
    new_state_counter = M->ns;
    destinations = (int *) malloc(sizeof(int) * num_new_states);
    
	dfaSetup(M->ns + num_new_states, len, indices); //add one new accept state
	exeps = (char *) malloc(max_exeps * (len + 1) * sizeof(char)); //plus 1 for \0 end of the string
	to_states = (int *) malloc(max_exeps * sizeof(int));
	statuces = (char *) malloc((M->ns + num_new_states + 2) * sizeof(char)); //plus 2, one for the new accept state and one for \0 end of the string
    
	// for each original state
	for (i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);
		k = 0;
		// for each transition out from current state (state i)
		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
                         tp->next)
						;
                    
					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[var] = '\0';


					// second copy to new accept state after removing lambda
					if (!isIncludeLambda(symbol, lambda, var)) {
						// no lambda send as it is
						to_states[k] = pp->to; // destination new accept state
						for (j = 0; j < var; j++)
							exeps[k * (len + 1) + j] = symbol[j];
						exeps[k * (len + 1) + var] = '0';
						exeps[k * (len + 1) + len] = '\0';
						k++;
					} else {
                        // add new state and add edge from current to new state on lambda2 (escape char)
                        destinations[new_state_counter - M->ns] = pp->to;
                        to_states[k] = new_state_counter;
                        for (j = 0; j < var; j++)
                            exeps[k * (len + 1) + j] = escapeLambda[j];
               			exeps[k * (len + 1) + var] = '1';
                        exeps[k * (len + 1) + len] = '\0';
                        k++;
                        new_state_counter++;
						// remove lambda then send copy to new accept state
						removeTransitionOnChar(symbol, lambda, var, charachters,
                                               &size);
						for (z = 0; z < size; z++) {
							// first copy of non-bamda char to original destination
							to_states[k] = pp->to;
							for (j = 0; j < var; j++)
								exeps[k * (len + 1) + j] = charachters[z][j];
                            exeps[k * (len + 1) + var] = '0';
							exeps[k * (len + 1) + len] = '\0';
							k++;
							free(charachters[z]);
						}
					}
			}
			pp = pp->next;
		} //end while
        
		dfaAllocExceptions(k);
		for (k--; k >= 0; k--){
			dfaStoreException(to_states[k], exeps + k * (len + 1));
        }
		dfaStoreState(sink);
        if (M->f[i] == 1)
            statuces[i] = '+';
        else
            statuces[i] = '-';
        
		kill_paths(state_paths);
	} // end for each original state
//    assert(new_state_counter == (num_new_states - 1));
	// add new states
    for (j = 0; j < var; j++)
        lambdaExtraBit[j] = lambda[j];
    lambdaExtraBit[var] = '0';
    lambdaExtraBit[len] = '\0';
    for (i = M->ns; i < new_state_counter; i++){
        dfaAllocExceptions(1);
        dfaStoreException(destinations[i - M->ns], lambdaExtraBit);
        dfaStoreState(sink);
        statuces[i] = '-';
    }
	statuces[new_state_counter] = '\0';
	result = dfaBuild(statuces);
    
    DFA *tmp = dfaProject(result, var);
    dfaFree(result);
    result = dfaMinimize(tmp);
    dfaFree(result);
    //	printf("dfaAfterRightTrimBeforeMinimize\n");
    //	dfaPrintGraphviz(result, len, indices);
	free(exeps);
	//printf("FREE ToState\n");
	free(to_states);
	//printf("FREE STATUCES\n");
	free(statuces);
	free(charachters);
    
//	free_ilt(states);
	free(lambda);
    free(escapeLambda);
    free(destinations);
    free(indices);
    free(symbol);
    free(lambdaExtraBit);
    
	return result;

}


int getNextStateOnLambda(DFA *M, int var, int *indices, char *lambda, int srcState, int sink){
    int j, nextState = -1;
    paths state_paths, pp;
	trace_descr tp;
    char *symbol = (char *) malloc((var + 1) * sizeof(char));
    
 
		state_paths = pp = make_paths(M->bddm, M->q[srcState]);
		// for each transition out from current state srcState
		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
                         tp->next)
						;
                    
					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[var] = '\0';
                
                
                // second copy to new accept state after removing lambda
                if (isIncludeLambda(symbol, lambda, var)) {
                    nextState = pp->to;
                    break;
                }
			}
			pp = pp->next;
		} //end while
		kill_paths(state_paths);
    free(symbol);
    return nextState;

}

DFA *dfa_pre_escape_single_finite_lang(DFA *M, int var, int *oldindices, char c, char escapeChar){
    DFA *result = NULL;
	char* escapeLambda = bintostr(escapeChar, var);
  	char* lambda = bintostr(c, var);
	int aux = 1;
    
	int maxCount = 0;
    
	paths state_paths, pp;
	trace_descr tp;
    
	int i, j, k;
    
	char *exeps;
	int *to_states;
	long max_exeps;
	char *statuces;
	int len = var + 1;
	int sink;
    
	int numOfChars;
	char** charachters;
    
	char *symbol;
    
  	int *indices = allocateArbitraryIndex(len); //indices is updated if you need to add auxiliary bits
    
	symbol = (char *) malloc((var + 1) * sizeof(char));
	maxCount = 0;
    
	max_exeps = 1 << len; //maybe exponential
	sink = find_sink(M);
	assert(sink > -1);
    
	numOfChars = 1 << var;
	charachters = (char**) malloc(numOfChars * (sizeof(char*)));
    
    
    
	dfaSetup(M->ns, len, indices); //add one new accept state
	exeps = (char *) malloc(max_exeps * (len + 1) * sizeof(char)); //plus 1 for \0 end of the string
	to_states = (int *) malloc(max_exeps * sizeof(int));
	statuces = (char *) malloc((M->ns + 2) * sizeof(char)); //plus 2, one for the new accept state and one for \0 end of the string
    
	// for each original state
	for (i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);
		k = 0;
		// for each transition out from current state (state i)
		while (pp) {
			if (pp->to != sink) {
				for (j = 0; j < var; j++) {
					//the following for loop can be avoided if the indices are in order
					for (tp = pp->trace; tp && (tp->index != indices[j]); tp =
                         tp->next)
						;
                    
					if (tp) {
						if (tp->value)
							symbol[j] = '1';
						else
							symbol[j] = '0';
					} else
						symbol[j] = 'X';
				}
				symbol[var] = '\0';
                
                to_states[k] = pp->to; // destination new accept state
                for (j = 0; j < var; j++)
                    exeps[k * (len + 1) + j] = symbol[j];
                exeps[k * (len + 1) + var] = '0';
                exeps[k * (len + 1) + len] = '\0';
                k++;
                // second copy to new accept state after removing lambda
                if (isIncludeLambda(symbol, escapeLambda, var)) {
                    int nextState = getNextStateOnLambda(M, var, oldindices, lambda, pp->to, sink);
                    if (nextState != -1){
                        to_states[k] = nextState; // destination new accept state
                        for (j = 0; j < var; j++)
                            exeps[k * (len + 1) + j] = lambda[j];
                        exeps[k * (len + 1) + var] = '1';
                        exeps[k * (len + 1) + len] = '\0';
                        k++;
                    }
                }
            }
			pp = pp->next;
		} //end while
        
		dfaAllocExceptions(k);
		for (k--; k >= 0; k--)
			dfaStoreException(to_states[k], exeps + k * (len + 1));
		dfaStoreState(sink);
        if (M->f[i] == 1)
            statuces[i] = '+';
        else
            statuces[i] = '-';
        
		kill_paths(state_paths);
	} // end for each original state
    //    assert(new_state_counter == (num_new_states - 1));
    statuces[M->ns] = '\0';
	result = dfaBuild(statuces);
    
    DFA *tmp = dfaProject(result, var);
    dfaFree(result);
    result = dfaMinimize(tmp);
    dfaFree(tmp);
    //	printf("dfaAfterRightTrimBeforeMinimize\n");
    //	dfaPrintGraphviz(result, len, indices);
	free(exeps);
	//printf("FREE ToState\n");
	free(to_states);
	//printf("FREE STATUCES\n");
	free(statuces);
	free(charachters);
    
	free(escapeLambda);
    free(lambda);
    free(indices);
    free(symbol);
    
	return result;
    
}

struct transition_list_type *getTransitionRelation(DFA* M,
                                                         int var, int* indices) {
	paths state_paths, pp;
	int sink = find_sink(M);
	char *symbol = (char *) malloc((var+1)*sizeof(char));
	struct transition_list_type *finallist = NULL;
	int i;
	for (i = 0; i < M->ns; i++) {
		state_paths = pp = make_paths(M->bddm, M->q[i]);        
		while (pp) {
			if (pp->to != sink) {
				finallist = transition_enqueue(finallist, i, pp->to);
                
			}
			pp = pp->next;
		} //end while
        
		kill_paths(state_paths);
	}
    
    //	printf("list of states reachable on \\s:");
    //	transition_print_ilt(finallist);
    //	printf("\n");
    
	free(symbol);
	return finallist;
}

struct int_list_type * dfsHelper(struct transition_list_type *transition_relation, struct int_list_type *visited, int current_state, int *p_loopFound, int loopState){
	visited = enqueue(visited, current_state);
	struct transition_type *tmp2;
	// for all transitions out from current state
	for (tmp2 = transition_relation->head; tmp2 != NULL; tmp2 = tmp2->next) {
		// if current transition is out from current state
		if (current_state == tmp2->from) {
            if (tmp2->to == loopState){
                *p_loopFound = TRUE;
                return visited;
            }
            //if destination is not visited before
            if (!check_value(visited, tmp2->to)){
                visited = dfsHelper(transition_relation, visited, tmp2->to, p_loopFound, loopState);
                if (*p_loopFound)
                    return visited;
            }
        }
	}
	return visited;
    
}

/**
 DFS to check if there is a loop in the automaton
 */
int dfs(DFA* M, int var, int* indices){
    
	struct transition_list_type *transition_relation = getTransitionRelation(M, var, indices);
	if (transition_relation == NULL)
		return FALSE;
	struct int_list_type *visited=NULL;
    int loopFound = FALSE;

    struct transition_type *tmp;
    //for each state check if it can reach itself. if so break and report a loop
    for (tmp = transition_relation->head; tmp != NULL && !loopFound; tmp = tmp->next) {
        dfsHelper(transition_relation, visited, tmp->from, &loopFound, tmp->from);
        if (visited != NULL){
            free_ilt(visited);
            visited = NULL;
        }
	}



	// free unneeded memory
	transition_free_ilt(transition_relation);
    //	printf("states that reach an accepting state on lambda: ");
    //	print_ilt(finallist);
    //	printf("\n");
	return loopFound;
}

/**
 checks if the length is finite using depth first seach for loops
 */
int isLengthFiniteDFS(DFA* M, int var, int *indices){
    return (1 - dfs(M, var, indices));
}






