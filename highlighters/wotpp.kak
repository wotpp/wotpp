# wpp syntax highlighting for kakoune

provide-module -override wpp %{
	add-highlighter shared/wpp regions
	add-highlighter shared/wpp/other default-region group

	add-highlighter shared/wpp/comment region -recurse '#\[' '#\[' '\]' group
	add-highlighter shared/wpp/comment/ fill comment

	add-highlighter shared/wpp/single-comment region '#' '$' group
	add-highlighter shared/wpp/single-comment/ fill comment

	add-highlighter shared/wpp/other/ regex %{\b(0x[_0-9a-fA-F]+|0b[_01]+)\b} 0:value

	add-highlighter shared/wpp/other/ regex "\b(drop|var|map|let|run|file|eval|assert|prefix|pipe|escape|error|log|slice|find|length)\b" 0:keyword
	add-highlighter shared/wpp/other/ regex "=|!|\.\.|->" 0:operator

	add-highlighter shared/wpp/raw_string region -match-capture %{r([^\s])"} %{"([^\s])} fill string
	add-highlighter shared/wpp/code_string region -match-capture %{c([^\s])"} %{"([^\s])} group
	add-highlighter shared/wpp/para_string region -match-capture %{p([^\s])"} %{"([^\s])} group

	add-highlighter shared/wpp/code_string/ fill string
	add-highlighter shared/wpp/para_string/ fill string

	add-highlighter shared/wpp/doublequote_string region '"' (?<!\\)(\\\\)*" group
	add-highlighter shared/wpp/singlequote_string region "'" (?<!\\)(\\\\)*' group

	add-highlighter shared/wpp/doublequote_string/ fill string
	add-highlighter shared/wpp/singlequote_string/ fill string

	add-highlighter shared/wpp/doublequote_string/ regex \\[\\ntr'"]|\\x[A-Fa-f0-9]{2}|\\b[01]{8} 0:keyword
	add-highlighter shared/wpp/singlequote_string/ regex \\[\\ntr'"]|\\x[A-Fa-f0-9]{2}|\\b[01]{8} 0:keyword

	add-highlighter shared/wpp/code_string/ regex \\[\\ntr'"]|\\x[A-Fa-f0-9]{2}|\\b[01]{8} 0:keyword
	add-highlighter shared/wpp/para_string/ regex \\[\\ntr'"]|\\x[A-Fa-f0-9]{2}|\\b[01]{8} 0:keyword
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


