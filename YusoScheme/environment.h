#pragma once
#include "common.h"
#include "expression.h"

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