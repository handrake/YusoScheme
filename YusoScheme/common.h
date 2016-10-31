#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "environment.h"
#include "expression.h"
enum ExpressionTypes {
	kSymbol = 0,
	kInt,
	kFloat,
	kString,
	kBool,
	kProc,
	kLambda,
	kList,
};

typedef std::unordered_map<std::string, Expression> EnvMap;
typedef const std::vector<Expression*> &Exps;
typedef Expression(*ProcType)(Expression &, Expression &);

