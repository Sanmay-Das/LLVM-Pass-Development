//=============================================================================
// FILE:
//    HelloWorld.cpp
//
// DESCRIPTION:
//    Visits all functions in a module, prints their names and the number of
//    arguments via stderr. Strictly speaking, this is an analysis pass (i.e.
//    the functions are not modified). However, in order to keep things simple
//    there's no 'print' method here (every analysis pass should implement it).
//
// USAGE:
//    New PM
//      opt -load-pass-plugin=libHelloWorld.dylib -passes="hello-world" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormatVariadic.h"

#include <map>

using namespace llvm;

//-----------------------------------------------------------------------------
// HelloWorld implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {

    void visitor(Function &fn) {
        errs() << "Liveness Analysis: " << fn.getName() << "\n";
    
        // Map pointers to source variable names
        std::map<Value*, std::string> pointerToVarName;
    
        // Data structures for analysis
        std::map<BasicBlock*, std::set<std::string>> UEVAR;
        std::map<BasicBlock*, std::set<std::string>> VARKILL;
        std::map<BasicBlock*, std::set<std::string>> LIVEOUT;
    
        // Step 1: Map pointers to variable names (from Alloca)
        for (auto& basicBlock : fn) {
            for (auto& instruction : basicBlock) {
                if (auto* allocaInst = dyn_cast<AllocaInst>(&instruction)) {
                    if (allocaInst->hasName()) {
                        // Map pointer (alloca) to variable name
                        pointerToVarName[allocaInst] = allocaInst->getName().str();
                    }
                }
            }
        }
    
        // Step 2: Calculate UEVAR and VARKILL for each basic block
        for (auto& basicBlock : fn) {
            std::set<std::string> uevarSet;
            std::set<std::string> varkillSet;
    
            for (auto& instruction : basicBlock) {
                // Handle Store Instructions (Defining Variables)
                if (auto* storeInst = dyn_cast<StoreInst>(&instruction)) {
                    Value* storedValue = storeInst->getValueOperand();
                    Value* pointer = storeInst->getPointerOperand();
    
                    // Check if the pointer is a known variable
                    if (pointerToVarName.find(pointer) != pointerToVarName.end()) {
                        std::string varName = pointerToVarName[pointer];
                        varkillSet.insert(varName);
                    }
    
                    // Check if the stored value is from a variable (UEVAR case)
                    if (auto* loadInst = dyn_cast<LoadInst>(storedValue)) {
                        Value* loadedPointer = loadInst->getPointerOperand();
                        if (pointerToVarName.find(loadedPointer) != pointerToVarName.end()) {
                            std::string varName = pointerToVarName[loadedPointer];
                            if (varkillSet.find(varName) == varkillSet.end()) {
                                uevarSet.insert(varName);
                            }
                        }
                    }
                }
                // Handle Load Instructions (Using Variables)
                else if (auto* loadInst = dyn_cast<LoadInst>(&instruction)) {
                    Value* pointer = loadInst->getPointerOperand();
                    if (pointerToVarName.find(pointer) != pointerToVarName.end()) {
                        std::string varName = pointerToVarName[pointer];
                        // Only UEVAR if not killed in this block
                        if (varkillSet.find(varName) == varkillSet.end()) {
                            uevarSet.insert(varName);
                        }
                    }
                }
            }
    
            UEVAR[&basicBlock] = uevarSet;
            VARKILL[&basicBlock] = varkillSet;
        }
    
        // Step 3: Iteratively calculate LIVEOUT using worklist
        bool changed = true;
        while (changed) {
            changed = false;
    
            for (auto& basicBlock : fn) {
                std::set<std::string> liveOutSet;
    
                // Collect LIVEOUT from successors
                for (auto succ = succ_begin(&basicBlock); succ != succ_end(&basicBlock); ++succ) {
                    BasicBlock* succBlock = *succ;
    
                    // LIVEOUT = (LIVEOUT - VARKILL) U UEVAR
                    std::set<std::string> tempOut = LIVEOUT[succBlock];
    
                    // Remove VARKILL
                    for (auto v : VARKILL[succBlock]) {
                        tempOut.erase(v);
                    }
                    // Add UEVAR
                    liveOutSet.insert(tempOut.begin(), tempOut.end());
                    liveOutSet.insert(UEVAR[succBlock].begin(), UEVAR[succBlock].end());
                }
    
                // Check if LIVEOUT changed
                if (LIVEOUT[&basicBlock] != liveOutSet) {
                    LIVEOUT[&basicBlock] = liveOutSet;
                    changed = true;
                }
            }
        }
    
        // Step 4: Print UEVAR, VARKILL, and LIVEOUT for each block
        for (auto& basicBlock : fn) {
            errs() << "----- " << basicBlock.getName() << " -----\n";
    
            // Print UEVAR
            errs() << "UEVAR: ";
            for (const auto& v : UEVAR[&basicBlock]) {
                errs() << v << " ";
            }
            errs() << "\n";
    
            // Print VARKILL
            errs() << "VARKILL: ";
            for (const auto& v : VARKILL[&basicBlock]) {
                errs() << v << " ";
            }
            errs() << "\n";
    
            // Print LIVEOUT
            errs() << "LIVEOUT: ";
            for (const auto& v : LIVEOUT[&basicBlock]) {
                errs() << v << " ";
            }
            errs() << "\n";
        }
    }
    
    
    

// New PM implementation
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    visitor(F);
    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-world") {
                    FPM.addPass(HelloWorld());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getHelloWorldPluginInfo();
}
