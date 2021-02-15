" Script:   wotpp.vim
" Homepage: http://github.com/Jackojc/wotpp
" License:  MPL-2.0
" Purpose:  Wot++ Syntax Highlighting

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

syn case match

syn keyword wppKeyword let run file eval assert prefix pipe escape error match
syn match wppOperator /\.\./
syn match wppOperator /->/
syn match wppOperator /\*/

syn keyword wppTodo TODO XXX FIXME HACK contained 
syn cluster wppCommentGrp contains=wppTodo,wppComment
syn region wppComment start='#\[' end=']' contains=@wppCommentGrp
syn sync match wppCommentSync groupthere wppComment /\(#\[|]\)/

syn match wppStringEscape /\\x[0-9a-fA-F_]\{2,}/
syn match wppStringEscape /\\b[01_]\{8,}/ 
syn match wppStringEscape /\\['"ntr]/ 
syn cluster wppStringGrp contains=wppStringEscape
syn region wppString start=/'/ skip=/\\./ end=/'/ contains=@wppStringGrp
syn region wppString start=/"/ skip=/\\./ end=/"/ contains=@wppStringGrp
syn match wppString /[prc]\(\S\)\(['"]\)\_.\{-}\2\1/ contains=@wppStringGrp
syn sync match wppStringSync groupthere wppString /\(\S"|"\S\)/

syn match wppHexLiteral /\<0x[0-9a-fA-F_]\+\>/
syn match wppBinLiteral /\<0b[01_]\+\>/

syn region wppBlock start='{' end='}' fold transparent

hi def link wppComment      Comment
hi def link wppKeyword      Keyword
hi def link wppOperator     Operator
hi def link wppString       String
hi def link wppHexLiteral   wppLiteral
hi def link wppBinLiteral   wppLiteral
hi def link wppLiteral      Number
hi def link wppTodo         Todo
hi def link wppStringEscape SpecialChar

let b:current_syntax = "wotpp"
