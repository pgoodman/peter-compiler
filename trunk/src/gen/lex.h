
#include <std-include.h>
#include <p-types.h>
#include <p-scanner.h>

extern G_Terminal lex_analyze(PScanner *);

G_Terminal lex_analyze(PScanner *S) {
    G_Terminal pterm = -1, term = -1;
    unsigned int n = 0, seen_accepting_state = 0;
    int cc, nc = 0, pnc = 0;
    scanner_mark_lexeme_start(S);
state_0:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 43: goto state_3;
        case 45: goto state_3;
        case 46: goto state_2;
        case 48: goto state_1;
        case 49: goto state_1;
        case 50: goto state_1;
        case 51: goto state_1;
        case 52: goto state_1;
        case 53: goto state_1;
        case 54: goto state_1;
        case 55: goto state_1;
        case 56: goto state_1;
        case 57: goto state_1;
        default: goto undo_and_commit;
    }
state_1:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = -1;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 46: goto state_2;
        case 48: goto state_1;
        case 49: goto state_1;
        case 50: goto state_1;
        case 51: goto state_1;
        case 52: goto state_1;
        case 53: goto state_1;
        case 54: goto state_1;
        case 55: goto state_1;
        case 56: goto state_1;
        case 57: goto state_1;
        case 69: goto state_5;
        case 101: goto state_5;
        default: goto undo_and_commit;
    }
state_2:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 48: goto state_4;
        case 49: goto state_4;
        case 50: goto state_4;
        case 51: goto state_4;
        case 52: goto state_4;
        case 53: goto state_4;
        case 54: goto state_4;
        case 55: goto state_4;
        case 56: goto state_4;
        case 57: goto state_4;
        default: goto undo_and_commit;
    }
state_3:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 46: goto state_2;
        case 48: goto state_1;
        case 49: goto state_1;
        case 50: goto state_1;
        case 51: goto state_1;
        case 52: goto state_1;
        case 53: goto state_1;
        case 54: goto state_1;
        case 55: goto state_1;
        case 56: goto state_1;
        case 57: goto state_1;
        default: goto undo_and_commit;
    }
state_4:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = -1;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 48: goto state_4;
        case 49: goto state_4;
        case 50: goto state_4;
        case 51: goto state_4;
        case 52: goto state_4;
        case 53: goto state_4;
        case 54: goto state_4;
        case 55: goto state_4;
        case 56: goto state_4;
        case 57: goto state_4;
        case 69: goto state_5;
        case 101: goto state_5;
        default: goto undo_and_commit;
    }
state_5:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 43: goto state_7;
        case 45: goto state_7;
        case 48: goto state_6;
        case 49: goto state_6;
        case 50: goto state_6;
        case 51: goto state_6;
        case 52: goto state_6;
        case 53: goto state_6;
        case 54: goto state_6;
        case 55: goto state_6;
        case 56: goto state_6;
        case 57: goto state_6;
        default: goto undo_and_commit;
    }
state_6:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = -1;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 48: goto state_6;
        case 49: goto state_6;
        case 50: goto state_6;
        case 51: goto state_6;
        case 52: goto state_6;
        case 53: goto state_6;
        case 54: goto state_6;
        case 55: goto state_6;
        case 56: goto state_6;
        case 57: goto state_6;
        default: goto undo_and_commit;
    }
state_7:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 48: goto state_6;
        case 49: goto state_6;
        case 50: goto state_6;
        case 51: goto state_6;
        case 52: goto state_6;
        case 53: goto state_6;
        case 54: goto state_6;
        case 55: goto state_6;
        case 56: goto state_6;
        case 57: goto state_6;
        default: goto undo_and_commit;
    }
undo_and_commit:
    if(!seen_accepting_state) {
        std_error("Scanner Error: Unable to match token.");
    }
    scanner_pushback(S, nc - pnc);
    scanner_mark_lexeme_end(S);
    return pterm;
}

