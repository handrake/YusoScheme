#pragma once
#include "common.h"
#include "environment.h"

class Expression {
private:
	ExpressionTypes type_;
	Environment *env_;
	ProcType proc_;
public:
	vector<Expression> list;
	string val;

	Expression(ExpressionTypes type = kSymbol) : type_(type), env_(nullptr) {}
	Expression(ExpressionTypes type, const string &val) : type_(type), val(val), env_(nullptr) {}
	Expression(ProcType proc) : type_(kProc), proc_(proc) {}

	ExpressionTypes get_type() const {
		return type_;
	}

	void set_env(Environment *env) {
		env_ = env;
	}

	Expression eval(Environment *env = global_env) {
		if (this->get_type() == kSymbol)
			return env->find(this->val)->at(this->val);
		else if (this->get_type() != kList) {
			return *this;
		}
		else if (this->list.empty()) {
			return env->find("nil")->at("nil");
		}
		else if (this->list[0].val == "define") {
			//return env[this->list[1].val] = this->list[2].eval();
		}
		return *this;
	}
	friend ostream& operator<<(ostream &os, const Expression &exp) {
		os << exp.val;
		return os;
	}
};