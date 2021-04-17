# EBNF highlighter

provide-module -override ebnf %{
	add-highlighter shared/ebnf regions
	add-highlighter shared/ebnf/other default-region group


	# Comments
	add-highlighter shared/ebnf/comment region -recurse '/\*' '/\*' '\*/' group
	add-highlighter shared/ebnf/comment/ fill comment

	add-highlighter shared/ebnf/single-comment region '//' '$' group
	add-highlighter shared/ebnf/single-comment/ fill comment


	# Operators and non terminals
	add-highlighter shared/ebnf/other/ regex "<\w+>" 0:identifier
	add-highlighter shared/ebnf/other/ regex "::=|\||\[|\]|\(|\)|\*|\+" 0:operator

	add-highlighter shared/ebnf/other/ regex "\{[0-9]+\}" 0:operator


	# Lexer terminals
	add-highlighter shared/ebnf/lexer region '\?' '\?' group
	add-highlighter shared/ebnf/lexer/ fill value


	# Strings
	add-highlighter shared/ebnf/doublequote_string region '"' (?<!\\)(\\\\)*" group
	add-highlighter shared/ebnf/singlequote_string region "'" (?<!\\)(\\\\)*' group

	add-highlighter shared/ebnf/doublequote_string/ fill string
	add-highlighter shared/ebnf/singlequote_string/ fill string

	add-highlighter shared/ebnf/doublequote_string/ regex \\[\\ntr'"] 0:keyword
	add-highlighter shared/ebnf/singlequote_string/ regex \\[\\ntr'"] 0:keyword
}

hook global BufCreate .*\.(ebnf) %{
	set-option buffer filetype ebnf
}

hook global WinSetOption filetype=ebnf %{
	require-module ebnf
}

hook -group ebnf-highlight global WinSetOption filetype=ebnf %{
	add-highlighter window/ebnf ref ebnf
	hook -once -always window WinSetOption filetype=.* %{ remove-highlighter window/ebnf }
}



