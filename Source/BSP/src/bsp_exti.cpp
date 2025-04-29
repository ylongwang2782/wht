#include "bsp_exti.hpp"
ExtiBase* ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI_NUM];
bool ExtiBase::is_bsp_init = false;
extern "C" {
    void EXTI0_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI0] != nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI0]->irq_handler();
        }
    }
    void EXTI1_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI1]!= nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI1]->irq_handler();
        } 
    }
    void EXTI2_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI2]!= nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI2]->irq_handler();
        } 
    }
    void EXTI3_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI3]!= nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI3]->irq_handler();
        } 
    }
    void EXTI4_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI4]!= nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI4]->irq_handler(); 
        } 
    }
    void EXTI5_9_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI5_9]!= nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI5_9]->irq_handler();
        } 
    }
    void EXTI10_15_IRQHandler(void){
        if(ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI10_15]!= nullptr){
            ExtiBase::exit[ExtiBase::EXTI_LINE::_EXTI10_15]->irq_handler(); 
        } 
    }
}