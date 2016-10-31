#include <cstddef>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <regex>
#include <numeric>
#include <cmath>

#define DEFINE_PROC_OP(op) \
([](const Expression &a, const Expression &b)->Expression { \
	if (a.type == b.type == kInt) { \
		return Expression(kInt, to_string((atol(a.val.c_str())) op (atol(b.val.c_str())))); \
	} \
	return Expression(kFloat, to_string((stod(a.val.c_str())) op (stod(b.val.c_str())))); \
})

#define DEFINE_PROC_COMP_OP(op) \
([](const Expression &a, const Expression &b)->Expression { \
	if (a.type == b.type == kInt) { \
		return Expression((atol(a.val.c_str())) op (atol(b.val.c_str()))); \
	} \
	return Expression((stod(a.val.c_str())) op (stod(b.val.c_str()))); \
})

using namespace std;

enum ExpressionTypes {
	kSymbol = 0,
	kInt,
	kFloat,
	kString,
	kBool,
	kProc,
	kNil,
	kLambda,
	kList,
};

class Expression;
class Environment;

typedef unordered_map<string, Expression> EnvMap;
typedef Expression(*ProcType)(const vector<Expression> &);

Environment *global_env;

class Expression {
private:
	Environment *env_;
public:
	vector<Expression> list;
	ExpressionTypes type;
	ProcType proc;
	string val;

	Expression(ExpressionTypes type = kSymbol) : type(type), env_(nullptr) {}
	Expression(ExpressionTypes type, const string &val) : type(type), val(val), env_(nullptr) {}
	Expression(ProcType proc) : type(kProc), proc(proc) {}
	Expression(bool b) : type(kBool), env_(nullptr) {
		this->val = b ? "#t" : "#f";
	}

	void set_env(Environment *env) {
		env_ = env;
	}

	Environment *get_env() const {
		return env_;
	}

	friend ostream& operator<<(ostream &os, const Expression &exp) {
		if (exp.type == kProc) {
			os << "<Proc>";
		}
		else if (exp.type == kList) {
			os << "<List> ";
			for (auto &i : exp.list) {
				os << i << " ";
			}
			os << endl;
		}
		else if (exp.type == kLambda) {
			os << "<Lambda>";
		}
		else {
			os << exp.val;
		}
		return os;
	}

	friend Expression operator==(const Expression &a, const Expression &b);
};

const Expression true_sym(kBool, "#t");
const Expression false_sym(kBool, "#f");
const Expression nil(kNil, "nil");

Expression operator==(const Expression &a, const Expression &b) {
	if (a.type == b.type && a.val == b.val)
		return true_sym;
	return false_sym;
}

class Environment {
private:
	Environment *outer_;
	EnvMap env_map_;

public:
	Environment(Environment *outer = nullptr) : outer_(outer) {}
	Environment(vector<Expression> &params, vector<Expression> &args, Environment *outer) : outer_(outer) {
		for (size_t i = 0; i < params.size(); ++i) {
			env_map_[params[i].val] = args[i];
		}
		this->show();
	}
	void update(const string &var, Expression exp) {
		env_map_.insert({ var, exp });
	}
	void remove(const string &var) { env_map_.erase(var); }

	EnvMap *find(const string &var) {
		auto result = env_map_.find(var);
		if (result != env_map_.end()) {
			return &env_map_;
		}
		if (outer_ != nullptr) {
			return outer_->find(var);
		}
		return nullptr;
	}

	void show() {
		for (auto &i : env_map_) {
			cout << i.first << ": " << i.second << endl;
		}
	}

	Expression &operator[] (const string &var) {
		return env_map_[var];
	}
};

Expression eval(Expression *exp, Environment *env = global_env) {
	if (exp->type == kSymbol) {
		return env->find(exp->val)->at(exp->val);
	}
	else if (exp->type != kList) {
		return *exp;
	}
	else if (exp->list.empty()) {
		return env->find("nil")->at("nil");
	}
	else if (exp->list[0].val == "symbol?") {
		return (env->find(exp->list[1].val) != nullptr) ? true_sym : false_sym;
	}
	else if (exp->list[0].val == "define") {
		return (*env)[exp->list[1].val] = eval(&exp->list[2], env);
	}
	else if (exp->list[0].val == "set!") {
		return env->find(exp->list[1].val)->at(exp->list[1].val) = eval(&exp->list[2], env);
	}
	else if (exp->list[0].val == "quote") {
		return exp->list[1];
	}
	else if (exp->list[0].val == "if") {
		Expression cond = eval(&exp->list[1], env);
		return (cond.val == "#t") ? eval(&exp->list[2], env) : eval(&exp->list[3], env);
	}
	else if (exp->list[0].val == "lambda") {
		exp->type = kLambda;
		exp->set_env(env);
		return *exp;
	}
	else if (exp->list[0].val == "begin") {
		for (size_t i = 1; i < exp->list.size() - 1; ++i) {
			eval(&exp->list[i], env);
		}
		return eval(&exp->list[exp->list.size() - 1], env);
	}
	else if (exp->list[0].val == "show") {
		env->show();
		return nil;
	}
	else {
		auto proc_name = exp->list[0].val;
		auto proc = env->find(proc_name)->at(proc_name);
		vector<Expression> args;

		for (auto &i = exp->list.begin() + 1; i != exp->list.end(); ++i) {
			args.push_back(eval(&*i, env));
		}
		if (proc.type == kLambda) {
			return eval(&proc.list[2], new Environment(proc.list[1].list, args, proc.get_env()));
		}
		else if (proc.type == kProc) {
			return proc.proc(args);
		}
	}
	return *exp;
}

list<string> tokenize(string &s) {
	list<string> tokens;

	s += " ";
	s = regex_replace(s, regex("[()]"), " $& ");

	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == ' ')
			continue;
		if (s[i] == '(' || s[i] == ')') {
			tokens.push_back(string(1, s[i]));
		}
		else {
			int j = s.find(' ', i);
			tokens.push_back(s.substr(i, j - i));
			i = j;
		}
	}

	return tokens;
}

Expression atom(const string &token) {
	if (regex_match(token, regex("#[tf]"))) {
		return Expression(kBool, token);
	}
	else if (regex_match(token, regex("-?\\d+"))) {
		return Expression(kInt, token);
	}
	else if (regex_match(token, regex("-?\\d+\\.\\d+"))) {
		return Expression(kFloat, token);
	}
	else if (regex_match(token, regex("\"[^\"]*\""))) {
		return Expression(kString, token);
	}
	return Expression(kSymbol, token);
}

Expression read_from_tokens(list<string> &tokens) {
	string token(tokens.front());

	tokens.pop_front();

	if (token == "(") {
		Expression exp(kList);

		while (tokens.front() != ")") {
			exp.list.push_back(read_from_tokens(tokens));
		}
		tokens.pop_front();
		return exp;
	}
	else if (token == ")") {
		throw logic_error("Unexpected )");
	}
	return atom(token);
}

Expression proc_add(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_OP(+));
}

Expression proc_sub(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_OP(-));
}

Expression proc_mul(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_OP(*));
}

Expression proc_div(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_OP(/));
}

Expression proc_greater(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_COMP_OP(<));
}

Expression proc_greater_equal(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_COMP_OP(<=));
}

Expression proc_less(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_COMP_OP(>));
}

Expression proc_less_equal(const vector<Expression> &e) {
	return accumulate(e.begin() + 1, e.end(), e.at(0), DEFINE_PROC_COMP_OP(>=));
}

Expression proc_equal(const vector<Expression> &e) {
	auto &head = e.front();
	for (auto &i = e.begin() + 1; i != e.end(); ++i) {
		if (head.type != i->type || head.val != i->val)
			return false_sym;
	}
	return true_sym;
}

Expression proc_abs(const vector<Expression> &e) {
	if (e.front().type == kInt) {
		long n = atol(e.front().val.c_str());
		return Expression(kInt, to_string(n > 0 ? n : -n));
	}
	double d = stod(e.front().val.c_str());
	return Expression(kFloat, to_string(d > 0 ? d : -d));
}

Expression proc_cons(const vector<Expression> &e) {
	Expression exp(kList);
	exp.list = e;
	return exp;
}

Expression proc_car(const vector<Expression> &e) {
	return e[0].list[0];
}

Expression proc_cdr(const vector<Expression> &e) {
	if (e[0].list.size() < 2)
		return nil;
	Expression exp(kList);
	exp.list = e[0].list;
	exp.list.erase(exp.list.begin());
	return exp;
}

Expression proc_length(const vector<Expression> &e) {
	return Expression(kInt, to_string(e[0].list.size()));
}

Expression proc_listp(const vector<Expression> &e) {
	return e[0].type == kList ? true_sym : false_sym;
}

Expression proc_not(const vector<Expression> &e) {
	if (e[0].type == kBool) {
		return (e[0].val == "#f") ? true_sym : false_sym;
	}
	return nil;
}

Expression proc_max(const vector<Expression> &e) {
	Expression max(e[0]);
	for (auto &i : e) {
		if (max.type == i.type == kInt) {
			if (atol(max.val.c_str()) < atol(i.val.c_str())) {
				max.val = i.val;
			}
		}
		else if (stod(max.val.c_str()) < stod(i.val.c_str())) {
			max.val = i.val;
			if (max.type == kInt) {
				max.type = kFloat;
			}
			else if (i.type == kInt) {
				max.type = kInt;
			}
		}
	}
	return max;
}

Expression proc_min(const vector<Expression> &e) {
	Expression min(e[0]);
	for (auto &i : e) {
		if (min.type == i.type == kInt) {
			if (atol(min.val.c_str()) > atol(i.val.c_str())) {
				min.val = i.val;
			}
		}
		else if (stod(min.val.c_str()) > stod(i.val.c_str())) {
			min.val = i.val;
			if (min.type == kInt) {
				min.type = kFloat;
			}
			else if (i.type == kInt) {
				min.type = kInt;
			}
		}
	}
	return min;
}

Expression proc_nullp(const vector<Expression> &e) {
	return (e[0].type == kList && e[0].list.empty()) ? true_sym : false_sym;
}

Expression proc_numberp(const vector<Expression> &e) {
	return (e[0].type == kInt || e[0].type == kFloat) ? true_sym : false_sym;
}

Expression proc_floor(const vector<Expression> &e) {
	if (e[0].type == kFloat) {
		return Expression(kFloat, to_string(floor(stod(e[0].val))));
	}
	return nil;
}

Expression proc_round(const vector<Expression> &e) {
	if (e[0].type == kFloat) {
		return Expression(kFloat, to_string(round(stod(e[0].val))));
	}
	return nil;
}

Expression proc_ceil(const vector<Expression> &e) {
	if (e[0].type == kFloat) {
		return Expression(kFloat, to_string(ceil(stod(e[0].val))));
	}
	return nil;
}

Expression proc_procedurep(const vector<Expression> &e) {
	return (e[0].type == kProc || e[0].type == kLambda) ? true_sym : false_sym;
}


Expression parse(string &program) {
	return read_from_tokens(tokenize(program));
}

Environment *standard_env() {
	Environment *env = new Environment();

	env->update("nil", nil);
	env->update("+", &proc_add);
	env->update("-", &proc_sub);
	env->update("*", &proc_mul);
	env->update("/", &proc_div);
	env->update("<", &proc_greater);
	env->update("<=", &proc_greater_equal);
	env->update(">", &proc_less);
	env->update(">=", &proc_less_equal);
	env->update("=", &proc_equal);
	env->update("abs", &proc_abs);
	env->update("cons", &proc_cons);
	env->update("car", &proc_car);
	env->update("cdr", &proc_cdr);
	env->update("list", &proc_cons);
	env->update("list?", &proc_listp);
	env->update("length", &proc_length);
	env->update("not", &proc_not);
	env->update("max", &proc_max);
	env->update("min", &proc_min);
	env->update("null?", &proc_nullp);
	env->update("number?", &proc_numberp);
	env->update("floor", &proc_floor);
	env->update("round", &proc_round);
	env->update("ceil", &proc_ceil);
	env->update("procedure?", &proc_procedurep);

	return env;
}

void repl(Environment *env) {
	string line;

	while (true) {
		cout << "lisp.cpp> ";
		getline(cin, line);
		if (regex_match(line, regex("[ ]*")))
			continue;
		cout << eval(&parse(line)) << endl;
	}
}

int main() {
	global_env = standard_env();
	repl(global_env);
}