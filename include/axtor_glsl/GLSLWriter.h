/*
 * GLSLWriter.h
 *
 *  Created on: ?
 *      Author: Simon Moll
 */

#ifndef OCLWRITER_H
#define OCLWRITER_H

#include <vector>
#include <set>
#include <string>

#include <llvm/Module.h>
#include <llvm/Value.h>
#include <llvm/Instruction.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Constants.h>
#include <llvm/Support/raw_ostream.h>

#include <axtor/writer/SyntaxWriter.h>
#include <axtor/CommonTypes.h>
#include <axtor/intrinsics/PlatformInfo.h>
#include <axtor/console/CompilerLog.h>
#include <axtor/util/llvmShortCuts.h>
#include <axtor/util/llvmConstant.h>
#include <axtor/util/stringutil.h>
#include <axtor/util/ResourceGuard.h>
#include <axtor/intrinsics/AddressIterator.h>
#include <axtor/backend/AddressSpaces.h>
#include <axtor/util/WrappedOperation.h>

#include <axtor/backend/generic/GenericCSerializer.h>
#include <axtor/backend/generic/GenericCWriter.h>

#include "GLSLModuleInfo.h"
#include "GLSLBackend.h"

#define INDENTATION_STRING "   "
#define GLSL_VERSION "140"

namespace axtor {

/*
* Generic SlangWriter interface
*/
class GLSLWriter : public GenericCWriter, protected GenericCSerializer
{
		friend class GLSLBlockWriter;
		friend class GLSLRedirectedWriter;
		friend class GLSLFragWriter;
		friend class GLSLMultiStageWriter;

private:
	GLSLModuleInfo & modInfo;
	PlatformInfo & platform;

protected:
	void writeFragmentCall(llvm::CallInst * call, IdentifierScope & funcContext);

	virtual void put(const std::string &  text);

	void writeShaderFunctionHeader(llvm::Function * func, IdentifierScope * funcContext);
	/*
	 * spills all of @func's arguments as globals attaching in/out with respect to @isReceiving
	 */
	void writeShaderArgumentDeclarations(const IdentifierScope & globals, llvm::Function * func, bool isVertToFragStage, bool isReceiving);

	/*
	 * maps a vector index [0..3] to a designator {x,y,z,w}
	 */
	std::string getComponentDesignator(int index);

public:

	enum ArgumentQualifier
	{
		INOUT = 0,
		IN = 1,
		OUT = 2
	};

	virtual void dump();

	virtual void print(std::ostream & out);

	/*
	 *  ##### Type System #####
	 */

	/*
	 * returns type symbols for default scalar types
	 */
	std::string getScalarType(const llvm::Type * type, bool signedInt = true);

	/*
	 * returns a argument type declaration attaching appropriate access qualifiers
	 */
	std::string getArgumentType(const llvm::Type * type, ArgumentQualifier access = INOUT);

	/*
	 * @override: the GenericCWriter implementation
	 */
	std::string getStructTypeDeclaration(const std::string & structName, const llvm::StructType * structType);

	/*
	 * generates a type name for @type
	 * if this is a pointer type, operate on its element type instead
	 */
	std::string getType(const llvm::Type * type);

	/*
	 * build a C-style declaration for @root of type @type
	 */
	std::string buildDeclaration(std::string root, const llvm::Type * type);


	/*
	* ##### DECLARATIONS / OPERATORS & INSTRUCTIONS ######
	 */

  	/*
    * writes a generic function header and declares the arguments as mapped by @funcContext
    */
	std::string getFunctionHeader(llvm::Function * func, IdentifierScope * funcContext);

	std::string getFunctionHeader(llvm::Function * func);

	virtual void writeVariableDeclaration(const VariableDesc & desc);

	virtual void writeFunctionDeclaration(llvm::Function * func, IdentifierScope * funcContext = NULL);

   /*
   * returns the string representation of a operator using @operands as operand literals
   */
   std::string getInstruction(llvm::Instruction * inst, StringVector operands);
   std::string getOperation(const WrappedOperation & op, StringVector operands);

   typedef std::vector<llvm::Value*> ValueVector;

   /*
    * return a dereferencing string for the next type node of the object using address
    */
	std::string dereferenceContainer(std::string root, const llvm::Type * type, AddressIterator *& address, IdentifierScope & funcContext, const llvm::Type *& oElementType);

	/*
	* return a name representing a dereferenced pointer
	 *if noImplicitDeref is false, the exact address of the value is returned
	 */
	std::string getDereffedPointer(llvm::Value * val, IdentifierScope & funcContext);
	std::string unwindPointer(llvm::Value * val, IdentifierScope & locals, bool & oDereferenced, const std::string * rootName);

	//std::string getReferenceTo(llvm::Value * val, IdentifierScope & funcContext);

	std::string getAllNullLiteral(const llvm::Type * type);
	/*
	* return the string representation of a constant
	*/
   std::string getLiteral(llvm::Constant * val);

   /*
	* tries to create a literal string it @val does not have a variable
	*/
   std::string getValueToken(llvm::Value * val, IdentifierScope & funcContext);

   /*
   * returns the string representation of a non-instruction value
   */
   std::string getNonInstruction(llvm::Value * value, IdentifierScope & funcContext);

   /*
   * returns the string representation of a ShuffleInstruction
   */
	std::string getShuffleInstruction(llvm::ShuffleVectorInst * shuffle, IdentifierScope & funcContext);

	/*
	 * returns the string representation of an ExtractElementInstruction
	 */
	std::string getExtractElementInstruction(llvm::ExtractElementInst * extract, IdentifierScope & funcContext);

	/*
	 * returns the string representation of an InsertElementInstruction
	 * if the vector value is defined this creates two instructions
	 */
	void writeInsertElementInstruction(llvm::InsertElementInst * insert, IdentifierScope & funcContext);

	/*
   * write a single instruction or atomic value as isolated expression
   */
   std::string getInstructionAsExpression(llvm::Instruction * inst, IdentifierScope & funcContext);

   /*
   * write a complex expression made up of elements from valueBlock, starting from root, writing all included insts to @oExpressionInsts
   */
   std::string getComplexExpression(llvm::BasicBlock * valueBlock, llvm::Value * root, IdentifierScope & funcContext, InstructionSet * oExpressionInsts = NULL);

   /*
    * writes a generic function header for utility functions and the default signature for the shade func
    */
   virtual void writeFunctionHeader(llvm::Function * func, IdentifierScope * funcContext = NULL);

	virtual void writeInstruction(const VariableDesc * desc, llvm::Instruction * inst, IdentifierScope & funcContext);

	virtual void writeIf(const llvm::Value * condition, bool negateCondition, IdentifierScope & locals);

	/*
	 * write a while for a post<checked loop
	 */
	void writePostcheckedWhile(llvm::BranchInst * branchInst, IdentifierScope & funcContext, bool negate);

	/*
	 * write a while for a postchecked loop, if oExpressionInsts != NULL dont write, but put all consumed instructions in the set
	 */
   virtual void writePrecheckedWhile(llvm::BranchInst * branchInst, IdentifierScope & funcContext, bool negate, InstructionSet * oExpressionInsts);

   virtual void writeInfiniteLoopBegin();

   virtual void writeInfiniteLoopEnd();

   virtual void writeReturnInst(llvm::ReturnInst * retInst, IdentifierScope & funcContext);


   virtual void writeFunctionPrologue(llvm::Function * func, IdentifierScope & funcContext);



   /*
    * dumps a generic vertex shader and all type&argument defs for the frag shader
    */
   GLSLWriter(ModuleInfo & _modInfo, const IdentifierScope & globals, PlatformInfo & _platform);

   GLSLModuleInfo & getModuleInfo();

protected:
   /*
    * writes a generic struct type declaration to the fragment shader
    */
   // virtual std::string getStructTypeDeclaration(const std::string & structName, const llvm::StructType * structType);
};

class GLSLBlockWriter : public GLSLWriter
{
	GLSLWriter & parent;

protected:
	virtual void put(const std::string &  text);

public:

	GLSLBlockWriter(GLSLWriter & _parent);

	/*
	 * redirects the call to the parent writer object. This is necessary to preserve function context sensitivity which is determined by the underlying function writer object
	 */
	void writeReturnInst(llvm::ReturnInst * retInst, IdentifierScope & locals);

	//pipe through to the base
	virtual void writeFunctionPrologue(llvm::Function * func, IdentifierScope & funcContext);

	virtual ~GLSLBlockWriter();
};

class GLSLRedirectedWriter : public GLSLWriter
{
	std::ostream & stream;

protected:
	virtual void put(const std::string &  text);

public:
	GLSLRedirectedWriter(GLSLWriter & _parent, std::ostream & _stream);

	virtual ~GLSLRedirectedWriter();
};

/*
 * writes into all stages at once
 * adds assignments to local variables for shader buffer pointers given as attributes at every function prologue in the fragment shader
 */
class GLSLMultiStageWriter : public GLSLWriter
{
	GLSLWriter * vert;
	GLSLWriter * frag;

protected:
	virtual void put(const std::string &  text);

public:
	/*
	 * taking possession of the writer objects
	 */
	GLSLMultiStageWriter(GLSLWriter & _parent, GLSLWriter * _vertWriter, GLSLWriter * _fragWriter);

	// spills assigns from pass-through device-pointer attributes to their real names (local vars) in fragment shader functions
	void writeFunctionPrologue(llvm::Function * func, IdentifierScope & funcContext);

	virtual ~GLSLMultiStageWriter();
};

class GLSLDummyWriter : public GLSLWriter
{
protected:
	virtual void put(const std::string &  text);

public:
	GLSLDummyWriter(GLSLWriter & parent);
};

class GLSLFragWriter : public GLSLRedirectedWriter
{
public:
	GLSLFragWriter(GLSLWriter & parent, std::ostream & fragStream);
	virtual void writeReturnInst(llvm::ReturnInst * retInst, IdentifierScope & funcContext);
};

}

#endif
