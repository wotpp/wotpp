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
