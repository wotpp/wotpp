# wpp syntax highlighting for kakoune

provide-module -override wpp %{
	add-highlighter shared/wpp regions
	add-highlighter shared/wpp/other default-region group


	add-highlighter shared/wpp/comment region -recurse '#\[' '#\[' '\]' group
	add-highlighter shared/wpp/comment/ fill comment


	add-highlighter shared/wpp/other/ regex "0b(0|1|_)+" 0:value
	add-highlighter shared/wpp/other/ regex "0x(A-Fa-f0-9|_)+" 0:value

	add-highlighter shared/wpp/other/ regex "let|\.\.|run|file|eval|assert|prefix|pipe|escape|error" 0:keyword
	add-highlighter shared/wpp/other/ regex "\(|\)|\{|\}" 0:operator


	add-highlighter shared/wpp/doublequote_string region '"' (?<!\\)(\\\\)*" group
	add-highlighter shared/wpp/singlequote_string region "'" (?<!\\)(\\\\)*' group

	add-highlighter shared/wpp/raw_doublequote_string region 'r"' (?<!\\)(\\\\)*" group
	add-highlighter shared/wpp/raw_singlequote_string region "r'" (?<!\\)(\\\\)*' group

	add-highlighter shared/wpp/paragraph_doublequote_string region 'p"' (?<!\\)(\\\\)*" group
	add-highlighter shared/wpp/paragraph_singlequote_string region "p'" (?<!\\)(\\\\)*' group

	add-highlighter shared/wpp/code_doublequote_string region 'c"' (?<!\\)(\\\\)*" group
	add-highlighter shared/wpp/code_singlequote_string region "c'" (?<!\\)(\\\\)*' group


	add-highlighter shared/wpp/doublequote_string/ fill string
	add-highlighter shared/wpp/singlequote_string/ fill string

	add-highlighter shared/wpp/raw_doublequote_string/ fill string
	add-highlighter shared/wpp/raw_singlequote_string/ fill string

	add-highlighter shared/wpp/paragraph_singlequote_string/ fill string
	add-highlighter shared/wpp/paragraph_doublequote_string/ fill string

	add-highlighter shared/wpp/code_singlequote_string/ fill string
	add-highlighter shared/wpp/code_doublequote_string/ fill string


	add-highlighter shared/wpp/doublequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword
	add-highlighter shared/wpp/singlequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword

	add-highlighter shared/wpp/raw_doublequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword
	add-highlighter shared/wpp/raw_singlequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword

	add-highlighter shared/wpp/paragraph_doublequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword
	add-highlighter shared/wpp/paragraph_singlequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword

	add-highlighter shared/wpp/code_doublequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword
	add-highlighter shared/wpp/code_singlequote_string/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword
}

hook global BufCreate .*\.(wpp) %{
	set-option buffer filetype wpp
}

hook global WinSetOption filetype=wpp %{
	require-module wpp
}

hook -group wpp-highlight global WinSetOption filetype=wpp %{
	add-highlighter window/wpp ref wpp
	hook -once -always window WinSetOption filetype=.* %{ remove-highlighter window/wpp }
}


