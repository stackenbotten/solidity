/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * UnusedFunctionArgumentPruner: Optimiser step that removes unused-parameters function.
 */

#include <libyul/optimiser/UnusedFunctionArgumentPruner.h>
#include <libyul/optimiser/NameCollector.h>
#include <libyul/optimiser/NameDispenser.h>
#include <libyul/optimiser/NameDisplacer.h>
#include <libyul/AsmData.h>

#include <libsolutil/CommonData.h>

using namespace std;
using namespace solidity::util;
using namespace solidity::yul;
using namespace solidity::langutil;

namespace
{

/**
 * First step of UnusedFunctionArgumentPruner: Find functions with whose parameters are not used in
 * its body.
 */
struct FindFunctionsWithUnusedParameters
{
	void operator()(FunctionDefinition const& _function)
	{
		auto namesFound = ReferencesCounter::countReferences(_function.body);

		TypedNameList reducedParameters;

		for (auto const& parameter: _function.parameters)
			if (namesFound.count(parameter.name))
				reducedParameters.push_back(parameter);

		if (reducedParameters.size() < _function.parameters.size())
		{
			functions.insert(_function.name);
			prunedTypeNames[_function.name] = move(reducedParameters);
		}
	}

	set<YulString> functions;
	map<YulString, TypedNameList> prunedTypeNames;
};

/**
 * Second step of UnusedFunctionArgumentPruner: replace all references to functions with unused
 * parameters with a new name.
 *
 * For example: `function f(x) -> y { y := 1 }` will be replaced with something
 * like : `function f_1(x) -> y { y := 1 }`  and all references to `f` by `f_1`.
 */
struct ReplaceFunctionName: public NameDisplacer
{
	explicit ReplaceFunctionName(
		NameDispenser& _dispenser,
		std::set<YulString> const& _namesToFree
	): NameDisplacer(_dispenser, _namesToFree) {}

	std::map<YulString, YulString>& translations() { return m_translations; }
};

/**
 * Third step of UnusedFunctionArgumentPruner: introduce a new function in the block with body of
 * the old one. Replace the body of the old one with a function call to the new one.
 *
 * For example: introduce a new function `f` with the same the body as `f_1`, but with reduced
 * parameters, i.e., `function f() -> y { y: = 1 }`. Now replace the body of `f_1` with a call to
 * `f`, i.e., `f_1(x) -> y { y := f() }`.
 */
class AddPrunedFunction
{
public:
	explicit AddPrunedFunction(
		set<YulString> const& _functions,
		map<YulString, TypedNameList> const& _prunedTypeNames,
		map<YulString, YulString> const&  _translations
	):
		m_functions(_functions),
		m_prunedTypeNames(_prunedTypeNames),
		m_translations(_translations)
	{
		for (auto const& _f: m_functions)
			m_inverseTranslations[m_translations.at(_f)] = _f;
	}

	void operator()(Block& _block)
	{
		iterateReplacing(_block.statements, [&](Statement& _s) -> optional<vector<Statement>>
		{
			if (holds_alternative<FunctionDefinition>(_s))
			{
				FunctionDefinition& _old = get<FunctionDefinition>(_s);
				if (m_inverseTranslations.count(_old.name))
					return addFunction(_old);
			}

			return nullopt;
		});
	}

private:
	vector<Statement> addFunction(FunctionDefinition& _old);

	set<YulString> const& m_functions;
	map<YulString, TypedNameList> const& m_prunedTypeNames;

	map<YulString, YulString> const& m_translations;
	map<YulString, YulString> m_inverseTranslations;
};

vector<Statement> AddPrunedFunction::addFunction(FunctionDefinition& _old)
{
	SourceLocation loc = _old.location;
	auto newName = m_inverseTranslations.at(_old.name);

	FunctionDefinition _new{
		loc,
		newName,
		m_prunedTypeNames.at(newName), // parameters
		_old.returnVariables,
		{loc, {}} // body
	};

	swap(_new.body, _old.body);

	// Replace the body of `f_1` by an assignment which calls `f`, i.e.,
	// `return_parameters = f(reduced_parameters)`
	{
		Assignment assignment;
		assignment.location = loc;

		// The LHS of the assignment.
		for (auto const& r: _old.returnVariables)
			assignment.variableNames.emplace_back(Identifier{loc, r.name});

		// RHS of the assignment
		FunctionCall call;
		call.location = loc;
		call.functionName = Identifier{loc, _new.name};
		for (auto const& p: m_prunedTypeNames.at(_new.name))
			call.arguments.emplace_back(Identifier{loc, p.name});

		assignment.value = make_unique<Expression>(move(call));

		_old.body.statements.emplace_back(move(assignment));
	}

	vector<Statement> statements;
	statements.emplace_back(move(_new));
	statements.emplace_back(move(_old));

	return statements;
}

} // anonymous namespace

void UnusedFunctionArgumentPruner::run(OptimiserStepContext& _context, Block& _block)
{
	FindFunctionsWithUnusedParameters _find;
	for (auto const& statement: _block.statements)
		if (holds_alternative<FunctionDefinition>(statement))
			_find(std::get<FunctionDefinition>(statement));

	if (_find.functions.empty())
		return;

	ReplaceFunctionName _replace{_context.dispenser, _find.functions};
	_replace(_block);

	AddPrunedFunction _add{_find.functions, _find.prunedTypeNames, _replace.translations()};
	_add(_block);
}
