" Vim syntax file
" Language:	CoroC 
" Maintainer:	Amal Cao (amalcaowei@gmail.com)
" Last Change:	2014 Jul 18

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Read the C syntax to start with
if version < 600
  source <sfile>:p:h/c.vim
else
  runtime! syntax/c.vim
endif

" CoroC extentions
syn keyword coType          __task_t __chan_t __refcnt_t
syn keyword coConstant      __CoroC_Null
syn keyword coStatement     __CoroC_Spawn __CoroC_Chan 
syn keyword coStatement     __CoroC_Yield __CoroC_Quit
syn keyword coStatement     __CoroC_Chan_Close __CoroC_Self __CoroC_Exit
syn keyword coConditional   __CoroC_Select
syn keyword coLabel         __CoroC_Case __CoroC_Default

hi def link coType          Type
hi def link coStatement     Statement
hi def link coConditional   Conditional
hi def link coLabel         Label
hi def link coConstant      Constant

let b:current_syntax = "co"

" vim: ts=8
