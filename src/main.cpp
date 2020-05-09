#include <sstream>
#include <string>
#include "clang/Basic/LangOptions.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Analysis/CFG.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

// Already exists here but inaccessible
// http://clang.llvm.org/doxygen/StmtPrinter_8cpp_source.html#l01101
static bool printExprAsWritten(raw_ostream &OS, Expr *E, const ASTContext *Context) {
	if (!Context)
		return false;
	bool Invalid = false;
	StringRef Source = Lexer::getSourceText(
		CharSourceRange::getTokenRange(E->getSourceRange()),
		Context->getSourceManager(), Context->getLangOpts(), &Invalid);
	if (!Invalid) {
		OS << Source;
		return true;
	}
	return false;
}

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
	MyASTVisitor(ASTContext &C, Rewriter &R) : TheContext(C), TheRewriter(R) {}

	bool VisitStmt(Stmt *s) {

		return true;
	}

	bool VisitForStmt(ForStmt *f) {
		llvm::errs() << "cond : ";
		printExprAsWritten(llvm::errs(), f->getCond(), &TheContext);
		llvm::errs() << "\n";
		llvm::errs() << "inc : ";
		printExprAsWritten(llvm::errs(), f->getInc(), &TheContext);
		llvm::errs() << "\n";

		llvm::errs() << "parenthesis : ";
		bool Invalid = false;
		StringRef Source = Lexer::getSourceText(
			CharSourceRange::getTokenRange(SourceRange(f->getLParenLoc(), f->getRParenLoc())),
			TheContext.getSourceManager(), TheContext.getLangOpts(), &Invalid);
		if (!Invalid)
			llvm::errs() << Source;
		llvm::errs() << "\n";

		return true;
	}

	bool VisitFunctionDecl(FunctionDecl *f) {
		if (TheContext.getSourceManager().isInSystemHeader(f->getSourceRange().getBegin())
		||  TheContext.getSourceManager().isInSystemHeader(f->getSourceRange().getEnd())
		|| (f->isTemplateInstantiation()
		&&  TheContext.getSourceManager().isInSystemHeader(f->getPointOfInstantiation())) )
			return true;
		// Only function definitions (with bodies), not declarations.
		if (f->hasBody()) {
			Stmt *funcBody = f->getBody();
			//CFG
			CFG::BuildOptions bo;
			bo.AddScopes=true;
			std::unique_ptr<CFG> sourceCFG = CFG::buildCFG(f, funcBody, &TheContext, bo);
			sourceCFG->print(llvm::errs(), LangOptions(), true);
			// Export dot files, works with clang built in debug mode only
			// sourceCFG->viewCFG(LangOptions());
		}
		return true;
	}

private:
	ASTContext &TheContext;
	Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
	MyASTConsumer(ASTContext &C, Rewriter &R) : Visitor(C, R) {}

	// Override the method that gets called for each parsed top-level
	// declaration.
	bool HandleTopLevelDecl(DeclGroupRef DR) override {
		for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
			// Traverse the declaration using our AST visitor.
			Visitor.TraverseDecl(*b);
			//(*b)->dump();
		}
		return true;
	}

private:
	MyASTVisitor Visitor;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
	MyFrontendAction() {}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
		TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		return std::make_unique<MyASTConsumer>(CI.getASTContext(), TheRewriter);
	}

private:
	Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
	CommonOptionsParser op(argc, argv, ToolingSampleCategory);
	ClangTool Tool(op.getCompilations(), op.getSourcePathList());

	// ClangTool::run accepts a FrontendActionFactory, which is then used to
	// create new objects implementing the FrontendAction interface. Here we use
	// the helper newFrontendActionFactory to create a default factory that will
	// return a new MyFrontendAction object every time.
	// To further customize this, we could create our own factory class.
	return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}

