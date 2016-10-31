#include <cstddef>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <regex>
#include <numeric>

#define DEFINE_PROC_OP(op) \
([](const Expression &a, const Expression &b)->Expression { \
	if (a.get_type() == b.get_type() == kInt) { \
		return Expression(kInt, to_string(atol(a.val.c_str()) op atol(b.val.c_str()))); \
	} \
	return Expression(kFloat, to_string(stod(a.val.c_str()) op stod(b.val.c_str()))); \
})

#define DEFINE_PROC_COMP_OP(op) \
([](const Expression &a, const Expression &b)->Expression { \
	if (a.get_type() == b.get_type() == kInt) { \
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
	ExpressionTypes type_;
	Environment *env_;
public:
	vector<Expression> list;
	ProcType proc;
	string val;

	Expression(ExpressionTypes type = kSymbol) : type_(type), env_(nullptr) {}
	Expression(ExpressionTypes type, const string &val) : type_(type), val(val), env_(nullptr) {}
	Expression(ProcType proc) : type_(kProc), proc(proc) {}
	Expression(bool b) : type_(kBool), env_(nullptr) {
		this->val = b ? "#t" : "#f";
	}

	ExpressionTypes get_type() const {
		return type_;
	}

	void set_env(Environment *env) {
		env_ = env;
	}

	friend ostream& operator<<(ostream &os, const Expression &exp) {
		if (exp.get_type() == kProc) {
			os << "<Proc>";
		}
		else if (exp.get_type() == kList) {
			os << "<List> ";
			for (auto &i : exp.list) {
				os << i << " ";
			}
			os << endl;
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
	if (a.get_type() == b.get_type() && a.val == b.val)
		return true_sym;
	return false_sym;
}

class Environment {
private:
	Environment *other_;
	EnvMap env_map_;

public:
	Environment(Environment *other = nullptr) : other_(other) {}
	void update(const string &var, Expression exp) {
		env_map_.insert({ var, exp });
	}
	void remove(const string &var) { env_map_.erase(var); }

	EnvMap *find(const string &var) {
		auto result = env_map_.find(var);
		if (result != env_map_.end()) {
			return &env_map_;
		}
		if (other_ != nullptr) {
			return other_->find(var);
		}
		return nullptr;
	}

	Expression &operator[] (const string &var) {
		return env_map_[var];
	}
};

Expression eval(Expression *exp, Environment *env = global_env) {
	if (exp->get_type() == kSymbol) {
		return env->find(exp->val)->at(exp->val);
	}
	else if (exp->get_type() != kList) {
		return *exp;
	}
	else if (exp->list.empty()) {
		return env->find("nil")->at("nil");
	}
	else if (exp->list[0].val == "define") {
		return (*env)[exp->list[1].val] = eval(&exp->list[2]);
	}
	else if (exp->list[0].val == "set!") {
		return env->find(exp->list[1].val)->at(exp->list[1].val) = eval(&exp->list[2]);
	}
	else if (exp->list[0].val == "quote") {
		return exp->list[1];
	}
	else if (exp->list[0].val == "if") {
		Expression cond = eval(&exp->list[1], env);
		return (cond.val == "#t") ? eval(&exp->list[2], env) : eval(&exp->list[3], env);
	}
	else if (exp->list[0].val == "lambda") {

	}
	else {
		auto proc_name = exp->list[0].val;
		auto proc = env->find(proc_name)->at(proc_name);
		if (proc.get_type() == kProc) {
			vector<Expression> args;
			for (auto &i = exp->list.begin() + 1; i != exp->list.end(); ++i) {
				args.push_back(eval(&*i, env));
			}
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
	return accumulate(e.begin(), e.end(), Expression(kInt, "0"), DEFINE_PROC_OP(+));
}

Expression proc_sub(const vector<Expression> &e) {
	return accumulate(e.begin(), e.end(), Expression(kInt, "0"), DEFINE_PROC_OP(-));
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
		if (head.get_type() != i->get_type() || head.val != i->val)
			return false_sym;
	}
	return true_sym;
}

Expression proc_abs(const vector<Expression> &e) {
	if (e.front().get_type() == kInt) {
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
	return e[0].get_type() == kList ? true_sym : false_sym;
}

Expression proc_symbolp(const vector<Expression> &e) {
	return e[0].get_type() == kSymbol ? true_sym : false_sym;
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
	env->update("symbol?", &proc_symbolp);

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