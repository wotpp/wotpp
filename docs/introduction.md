# Beginner's Guide to Wot++

## String types

Strings are the data we focus on in this language and there are different types we have here to make working with them easier. Strings can be identified by a `""` or a `''` which is pretty nice.

### Raw Strings

Raw strings are as simple as they come what gets entered is what gets stored as a string. The two ways to call it are either `""` which implicitly treats the string as a raw string but if you wish to be explicit you can use. `r#" #"`

Input:

```wot++
r#"
    Hello world.
#"
```

Would produce an output of:
`\n\tHello world.\n`

### Paragraph Strings

Now what a paragraph string does is removes leading and trailing whitespace. It's syntax is very similar to the raw string it is. `p#" #"`

Input:

```wot++
p#"
    Hello world.
p#"
```

This makes the output:
`Hello world`

### Code Strings

This is for when you want to store a string that is code It will remove common leading white space. Like the last two it follows as such. `c#" #"`

Input:

```wot++
    c#"
        Hello World.
    #"
```

The output is:
` \tHello World. `

### Functions

A function is a command which takes arguments and return an expression. The syntax for making a function is ` let name(args) expression ` args is optional it can be a void function.  so the function ` let foo() "foo" ` returns. `foo`

Code blocks are denoted via the use of curly brackets "{}"  This is used for when you want to state that something is apart of an expression of a function and not a function itself.

Input:

```wot++
let foo(){
    let boo() "boo"
    let pop() "pop"
    "baz"
}
```

Output:
` baz `
This prints baz and declares two functions.
Calling a function is simply done by saying. ` name(args) `

Prefixes:
The prefix is a way to group code together the syntax for this is:

```wot++
prefix name/ {
    let foo() "foo" 
    let boo() "boo"
 } 
```

You can now call these two functions by writing `name/foo()` and ` name/boo() ` This will be very useful for organisation.

### Concatenation

Concatenation is represented by two dots as so ` .. ` this can be used to join two expression as so ` expression .. expression ` it can also be used to concatenate the return of a function call as such. ` foo() .. boo() `

### Comments

Comments are always useful so we included them for you they are multiline and can be nested. ` #[ comments are great ] `
