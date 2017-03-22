/*
 * RestructuringPass.h
 *
 *  Created on: 20.02.2011
 *      Author: Simon Moll
 */

#ifndef RESTRUCTURINGPASS_HPP_
#define RESTRUCTURINGPASS_HPP_

#include <axtor/config.h>

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <axtor/CommonTypes.h>
#include <axtor/ast/ASTFactory.h>
#include <axtor/util/AnalysisStruct.h>

#include <axtor/parsers/IfParser.h>
#include <axtor/parsers/LoopParser.h>

#include <axtor/parsers/PrimitiveParser.h>
#include <axtor/pass/AnalysisProvider.h>

namespace axtor {

	struct Parsers
	{
		PrimitiveParser * loopParser;
		PrimitiveParser * ifParser;

		Parsers() :
			loopParser(LoopParser::getInstance()),
			ifParser(IfParser::getInstance())
		{}
	};

	class RestructuringPass : public llvm::ModulePass, public AnalysisProvider
	{
		Parsers parsers;

		ast::ASTMap astMap;

		/*
		 * call processBasicBlock until the exit block is returned
		 */
		ast::ControlNode * processRegion(bool enteredLoop, const ExtractorRegion & region, AnalysisStruct & analysis, BlockSet & visited);

		/*
		 * translates a basic block and returns the expected successor block, if any
		 */
		llvm::BasicBlock * processBasicBlock(bool enteredLoop, llvm::BasicBlock * bb, const ExtractorContext & context, AnalysisStruct & analysis, BlockSet & visited, ast::ControlNode *& oNode);

		ast::FunctionNode * runOnFunction(llvm::Function & func);

	public:

		virtual void rebuildAnalysisStruct(llvm::Function & func, AnalysisStruct & analysis);

		static char ID;

		RestructuringPass() :
			llvm::ModulePass(ID)
		{}

		virtual void getAnalysisUsage(llvm::AnalysisUsage & usage) const;

		bool runOnModule(llvm::Module & M);

                llvm::StringRef getPassName() const;

		virtual void releaseMemory();

		/*
		 * required to obtain the root node through the llvm pass interface
		 */
		const ast::ASTMap & getASTs() const;
	};
}

#endif /* RESTRUCTURINGPASS_HPP_ */
