# wpp syntax highlighting for kakoune

provide-module -override wpp %{
	add-highlighter shared/wpp regions
	add-highlighter shared/wpp/other default-region group

	add-highlighter shared/wpp/comment  region -recurse '\[' '\[' '\]' group
	add-highlighter shared/wpp/dqstring region '"' (?<!\\)(\\\\)*" group
	add-highlighter shared/wpp/sqstring region "'" (?<!\\)(\\\\)*' group

	add-highlighter shared/wpp/comment/  fill comment
	add-highlighter shared/wpp/dqstring/ fill string
	add-highlighter shared/wpp/sqstring/ fill string

	add-highlighter shared/wpp/dqstring/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword
	add-highlighter shared/wpp/sqstring/ regex \\[\\ntvr'"]|\\x[A-Fa-f0-9]+|\\b[01]+ 0:keyword

	add-highlighter shared/wpp/other/ regex "let|;|\?|\.\.|exec|file|source|replace|assert|namespace" 0:keyword
	add-highlighter shared/wpp/other/ regex "\(|\)" 0:operator
	add-highlighter shared/wpp/other/ regex "\{|\}|<|>" 0:comment
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


