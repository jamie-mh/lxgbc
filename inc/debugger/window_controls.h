#pragma once
#include "window.h"

class WindowControls final : public Window {
    public:
        explicit WindowControls(Debugger &);

        void render() override;
        const char *title() const override;

    private:
        void step_into();
        void step_over();
        void run_to_next();

        static bool is_subroutine_call(uint8_t opcode) ;
        static bool is_jump_call(uint8_t opcode) ;
        static bool is_jump_signed(uint8_t opcode) ;
        static bool is_return(uint8_t opcode) ;
};
