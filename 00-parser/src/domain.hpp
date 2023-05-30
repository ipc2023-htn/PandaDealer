#ifndef __DOMAIN
#define __DOMAIN

#include <vector>
#include <map>
#include <set>
#include <string>
#include "sortexpansion.hpp"

using namespace std;


const string dummy_equal_literal = "__equal";
const string dummy_ofsort_literal = "__ofsort";
const string dummy_function_type = "__none";
const string numeric_funtion_type = "number";
const string method_precondition_action_name = "__method_precondition_";
const string immediate_method_precondition_action_name = "__immediate_method_precondition_";


struct literal{
	bool positive;
	bool isConstantCostExpression;
	bool isCostChangeExpression;
	string predicate;
	vector<string> arguments;
	int costValue;
};

struct conditional_effect{
	vector<literal> condition;
	literal effect;

	conditional_effect(vector<literal> cond, literal eff);
};

struct task{
	string name;
	int number_of_original_vars; // the first N variables are original, i.e. exist in the HDDL input file. The rest is artificial and was added by this parser for compilation
	vector<pair<string,string>> vars;
	vector<literal> prec;
	vector<literal> eff;
	vector<conditional_effect> ceff;
	vector<literal> constraints;
	vector<literal> costExpression;

	bool artificial;

	void check_integrity();
};

struct plan_step{
	string task;
	string id;
	vector<string> args;

    bool operator< (const plan_step& ps) const {
        return (id < ps.id);
    }
};

struct method{
	method(){adj_matrix_computed = false;};
	string name;
	vector<pair<string,string>> vars;
	string at;
	vector<string> atargs;
	vector<plan_step> ps;
	vector<literal> constraints;
	vector<pair<string,string>> ordering;

	void check_integrity();
	
	bool is_sub_group(set<string> & sset, set<string> & beforeID, set<string> & afterID);

	private:
		map<string,set<string>> adj_matrix;
		bool adj_matrix_computed;
		void compute_adj_matrix();
};


// sort name and set of elements
extern map<string,set<string> > sorts;
extern vector<method> methods;
extern vector<task> primitive_tasks;
extern vector<task> abstract_tasks;
extern map<string, task> task_name_map;

void flatten_tasks(bool compileConditionalEffects, bool linearConditionalEffectExpansion, bool encodeDisjunctivePreconditionsInMethods);
void parsed_method_to_data_structures(bool compileConditionalEffects, bool linearConditionalEffectExpansion, bool encodeDisjunctivePreconditionsInMethods);
void reduce_constraints();
void clean_up_sorts();
void remove_unnecessary_predicates();


set<string> compute_constants_in_domain();

#endif
