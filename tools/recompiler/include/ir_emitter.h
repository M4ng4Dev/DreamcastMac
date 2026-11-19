#pragma once
#include "sh4_disasm.h"
#include <string>
#include <vector>
#include <ostream>

namespace dc::recomp {

class CppEmitter {
public:
    static void emit_header(std::ostream& out);
    static void emit_function(std::ostream& out, const FunctionBlock& func);
    static void emit_dispatcher(std::ostream& out, const std::vector<FunctionBlock>& functions);
    static void emit_complete_translation(std::ostream& out, const std::vector<FunctionBlock>& functions);
};

} // namespace dc::recomp
