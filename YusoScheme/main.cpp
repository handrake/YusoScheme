#include <cstddef>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <regex>
#include "common.h"
#include "environment.h"
#include "expression.h"

#define DEFINE_PROC_OP(op)																	\
	([](Expression &a, Expression &b)->Expression {											\
		if (a.get_type() == b.get_type() == kInt) {											\
			return Expression(kInt, to_string(atol(a.val.c_str()) op atol(b.val.c_str())));	\
		}																					\
	return Expression(kFloat, to_string(stod(a.val.c_str()) op stod(b.val.c_str())));		\
})

using namespace std;

class Expression;
class Environment;

Environment *global_env;

bool IsPrimitiveType(ExpressionTypes type) {
	switch (type) {
	case kSymbol:
	case kInt:
	case kFloat:
	case kString:
	case kBool:
		return true;
	}
	return false;
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
	else if (regex_match(token, regex("\\d+"))) {
		return Expression(kInt, token);
	}
	else if (regex_match(token, regex("\\d+\\.\\d+"))) {
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

Expression parse(string &program) {
	return read_from_tokens(tokenize(program));
}

Environment *standard_env() {
	Environment *env = new Environment();

	env->update("+", Expression(DEFINE_PROC_OP(+)));
	env->update("-", Expression(DEFINE_PROC_OP(-)));
	env->update("*", Expression(DEFINE_PROC_OP(*)));
	env->update("/", Expression(DEFINE_PROC_OP(/)));

	return env;
}

void repl(Environment *env) {
	string line;

	while (true) {
		cout << "lisp.cpp> ";
		getline(cin, line);
		if (regex_match(line, regex("[ ]*")))
			continue;
		cout << parse(line).eval() << endl;
	}
}

int main() {
	global_env = standard_env();
	repl(global_env);
}