
syntax match	comment	"\/\/.*$"
syntax region	comment	start="\/\*" end="\*\/"

syntax keyword	keyload	load_op load_so load_type load_builtin
syntax keyword	newkey	newop newtype
syntax keyword	bool	TRUE FALSE

syntax match	str	"\"\(\\.\|[^\"\\]\)*\""
syntax match	id	"[a-zA-Z_][a-zA-Z_0-9]*"
syntax match	num	"[0-9][0-9]*"
syntax match	num	"[0-9][0-9]*\.[0-9]*"

syntax match	bkload	"[()]"
syntax match	bracket	"[\[\]]"
syntax match	arrow	"[<=:\~\.]"

hi link comment Comment

hi link keyload Keyword
hi link bkload Keyword

hi link bracket Constant
hi link arrow Constant

hi link newkey NonText
hi link bool NonText
hi link num NonText
hi link str NonText

hi link id Text

