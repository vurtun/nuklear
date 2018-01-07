/// ## About
/// - _stddoc.c_ is a tiny documentation generator for 60 programming languages.
/// - This page sample was auto-generated from the code comments found in `stddoc.c` file.
///
/// ## How does it work?
/// - Markdeep code comments are extracted from stdin and printed into stdout as a HTML file.
///
/// ## Supported languages
/// - `/// Three slashes comment` [ActionScript, AngelScript, C (C99), C#, C++, ChaiScript, D,
///   GameMonkey, GML, Go, Java, JavaScript, JetScript, jtc, Jx9, Kotlin, Neko, Object Pascal (Delphi),
///   Objective-C, Pawn, PHP, QuakeC, Rust, SASS, Scala, Squirrel, Swift, Vala, Wren, Xojo].
/// - `--- Three dashes comment` [Ada, AppleScript, Eiffel, Euphoria, Haskell, Lua, Occam,
///   PL/SQL, PSL, SGML, SPARK, SQL, Terra, TSQL, VHDL].
/// - `### Three hashes comment` [AWK, Bash, Bourne shell, C shell, Cobra, Maple, Maple,
///   Perl, Perl6, PowerShell, Python, R, Ruby, Seed7, Tcl]. 
///
/// ## Usage
/// - `stddoc < source.code > documentation.html`
///
/// ## Changelog
/// 2018/01/07
/// :  Initial version (_v1.0.0_)
///
/// ## License
/// - rlyeh, unlicensed (~public domain).

#include <stdio.h>

int main() {
    printf("%s\n", "<meta charset='utf-8' emacsmode='-*- markdown -*-'>");
    printf("%s\n", "<link rel='stylesheet' href='https://casual-effects.com/markdeep/latest/apidoc.css?'>");

    for( int fsm_S = 0, fsm_D = 0, fsm_H = 0; !feof(stdin); ) {
        int chr = getc(stdin);
        if( fsm_S > 3 || fsm_D > 3 || fsm_H > 3 ) {
            putc(chr, stdout);
            if( chr != '\r' && chr != '\n' ) continue;
        }
        /**/ if( fsm_S <= 2 && chr == '/' && !fsm_D && !fsm_H ) fsm_S++;
        else if( fsm_S == 3 && chr == ' ' && !fsm_D && !fsm_H ) fsm_S++;
        else if( fsm_D <= 2 && chr == '-' && !fsm_S && !fsm_H ) fsm_D++;
        else if( fsm_D == 3 && chr == ' ' && !fsm_S && !fsm_H ) fsm_D++;
        else if( fsm_H <= 2 && chr == '#' && !fsm_S && !fsm_D ) fsm_H++;
        else if( fsm_H == 3 && chr == ' ' && !fsm_S && !fsm_D ) fsm_H++;
        else fsm_S = fsm_D = fsm_H = 0;
    }

    printf("%s\n", "<script>markdeepOptions={tocStyle:'medium'};</script>");
    printf("%s\n", "<!-- Markdeep: --><script src='https://casual-effects.com/markdeep/latest/markdeep.min.js?'></script>");
}

///
/// ## **Example page!**
/// 
/// Imaginary documentation page. Here would be some introduction text.
/// 
/// The table of contents that Markdeep produces is stuffed on the right side, 
/// if the browser window is wide enough. Otherwise it is hidden.
/// 
/// ### Basic Markdeep
/// 
/// Regular styling like **bold**, _italics_, ~~strikethrough~~, `inline code`, etc. Lists as:
/// 
/// * A
/// * Bullet
/// * List
/// 
/// And:
/// 
/// 1. A
/// 1. Numbered
/// 1. List!
/// 
/// Symbol substitutions: a 45-degree turn; som x -> y arrows; some whoa ==> fancy <==> arrows.
/// 
/// Is this a definition list?
/// :   Looks like one to me
/// Is that right?
/// :   Possibly!
/// 
/// And a code listing:
/// 
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// int main()
/// {
///     return 1;
/// }
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// 
/// 
/// ### Tables
/// 
/// Thing Name              | Description        |Notes
/// ------------------------|--------------------|-----
/// Yes                     | Yup!               |
/// No                      | Nope :(            |
/// FileNotFound            | Doesn't find files | Pass `-sFIND_FILE=maybe` to maybe find them
/// 
/// 
/// ### Diagrams
/// 
/// *******************************************  Here's a text to the right of the diagram,
/// * +-----------------+           .-.       *  ain't that fancy. Pretty fancy indeed, I
/// * |\                |        .-+   |      *  must say! Markdeep diagrams are generally
/// * | \     A-B   *---+--> .--+       '--.  *  enclosed into a rectangle full made of `*`
/// * |  \              |   |     Cloud!    | *  symbols; and are "drawn" using ASCII-art
/// * +---+-------------+    '-------------'  *  style, with `- | + / \ * o` etc.
/// *******************************************  Suh-weet!
/// 
/// Another random diagram, just because:
/// 
/// ********************
/// *    +-+-+-+-*-o   *
/// *   / / ^ /        *
/// *  / v / /         *
/// * +-+-+-+          *
/// ********************
/// 
/// ### Special notes
/// 
/// !!! Note
///     Hey I'm a note. Don't mind me, I'm just sitting here.
/// 
/// !!! WARNING
///     I'm a warning, perhaps. *Something might happen!*
/// 
/// !!! Error: Never Pass `nullptr` to a Shader
///     Invoking a shader with a null argument can seg fault.
///     This is a multi-line admonition.
/// 
///     Seriously, don't call shaders like that.
/// 
/// ### Embedding HTML
/// 
/// <pre>
/// This is an embedded html node by the way!
/// </pre>
/// 
/// ## Credits
/// - API doc style created by [Aras Pranckevičius](https://github.com/aras-p)
/// - Markdeep by [Morgan McGuire](https://casual-effects.com/markdeep/).
