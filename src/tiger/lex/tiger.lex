%filenames = "scanner"
%x COMMENT STR IGNORE CONVERT

%%
R"(/*)" {
    adjust();
    push(StartCondition__::COMMENT);
}
<COMMENT> R"(/*)" {
    adjust();
    push(StartCondition__::COMMENT);
}
<COMMENT> R"(*/)" {
    adjust();
    popStartCondition();
}
<COMMENT>. {
    adjust();
}
<COMMENT>\n {
    adjust();
}

"var" {adjust(); return Parser::VAR;}
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"nil" {adjust(); return Parser::NIL;}
"end" {adjust(); return Parser::END;}
"function" {adjust(); return Parser::FUNCTION;}
"else" {adjust(); return Parser::ELSE;}
"in" {adjust(); return Parser::IN;}
"let" {adjust(); return Parser::LET;}
"type" {adjust(); return Parser::TYPE;}
"while" {adjust(); return Parser::WHILE;}
"do" {adjust(); return Parser::DO;}
"array" {adjust(); return Parser::ARRAY;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
[a-zA-Z][a-z0-9A-Z_]* {adjust(); return Parser::ID;}

["] {
    adjust();
    more();
    minus_for_convert_char = 0;
    push(StartCondition__::STR);
}
<STR>[a-zA-Z0-9\+\-\*\/\. !] {
    more();
}
<STR>">" {
    more();
}
<STR>"<" {
    more();
}
<STR>["] {
    std::string current_string = matched();
    adjustByLen(current_string.size() + minus_for_convert_char - 1);
    current_string = current_string.substr(1, current_string.size() - 2); //remove the ""
    setMatched(current_string);
    popStartCondition();
    return Parser::STRING;
}

<STR>[\\] {
    more();
    push(StartCondition__::CONVERT);
}
<CONVERT>[\^][A-Z] {
    more();
    std::string current_string = matched();
    char last_matched_letter = current_string[current_string.size() - 1];
    current_string = current_string.substr(0, current_string.size() - 3); //remove the \^X
    current_string += char(last_matched_letter - 'A' + 1); //add real \^X, remember \^A is 1
    minus_for_convert_char += 2;
    setMatched(current_string);
    popStartCondition();
}
<CONVERT>[0-9]+ {
    more();
    std::string current_string = matched();
    int inverse_matched_count = 0;
    for (int i = current_string.size() - 1; i >= 0; i--) {
        if (current_string[i] <= '9' && current_string[i] >= '0') inverse_matched_count ++;
        else break;
    }
    minus_for_convert_char += inverse_matched_count;
    std::string convert_number_string = current_string.substr(current_string.size() - inverse_matched_count);
    current_string = current_string.substr(0, current_string.size() - inverse_matched_count - 1); //remove the \^X
    int asc2_number = atoi(convert_number_string.c_str());
    current_string += asc2_number; //add real \[0-9]+
    setMatched(current_string);
    popStartCondition();
}
<CONVERT>[t] {
    more();
    minus_for_convert_char ++;
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 2); //remove the \t
    current_string += '\t'; //add real \t
    setMatched(current_string);
    popStartCondition();
}
<CONVERT>[\\] {
    more();
    minus_for_convert_char ++;
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 2); //remove the \\
    current_string += '\\'; //add real \\
    setMatched(current_string);
    popStartCondition();
}
<CONVERT>[n] {
    more();
    minus_for_convert_char ++;
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 2); //remove the \n
    current_string += '\n'; //add real \n
    setMatched(current_string);
    popStartCondition();
}
<CONVERT>["] {
    more();
    minus_for_convert_char ++;
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 2); //remove the \"
    current_string += '\"'; //add real "
    setMatched(current_string);
    popStartCondition();
}
<CONVERT>[\n] {
    more();
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 2); //remove the \n
    minus_for_convert_char += 2;
    setMatched(current_string);
    push(StartCondition__::IGNORE);
}
<IGNORE>[\\] {
    more();
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 1); //remove everything except \
    minus_for_convert_char ++;
    setMatched(current_string);
    popStartCondition();
    popStartCondition();
}
<IGNORE>. {
    more();
    std::string current_string = matched();
    current_string = current_string.substr(0, current_string.size() - 1); //remove everything except \
    minus_for_convert_char ++;
    setMatched(current_string);
}


[0-9]+ {adjust(); return Parser::INT;}

\, {adjust(); return Parser::COMMA;}
\: {adjust(); return Parser::COLON;}
\; {adjust(); return Parser::SEMICOLON;}
\( {adjust(); return Parser::LPAREN;}
\) {adjust(); return Parser::RPAREN;}
\[ {adjust(); return Parser::LBRACK;}
\] {adjust(); return Parser::RBRACK;}
\{ {adjust(); return Parser::LBRACE;}
\} {adjust(); return Parser::RBRACE;}
\. {adjust(); return Parser::DOT;}
\+ {adjust(); return Parser::PLUS;}
\- {adjust(); return Parser::MINUS;}
\* {adjust(); return Parser::TIMES;}
\/ {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");
    std::cout << "Matched = " << matched() << std::endl;
}
