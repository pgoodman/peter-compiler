
#ifndef _P_LEX_H_
#define _P_LEX_H_
#include <ctype.h>
#include <std-include.h>
#include <p-types.h>
#include <p-scanner.h>

extern G_Terminal lex_parser(PScanner *);

G_Terminal lex_parser(PScanner *S) {
    G_Terminal pterm = -1, term = -1;
    unsigned int n = 0, seen_accepting_state = 0;
    int cc, nc = 0, pnc = 0;
    scanner_skip(S, &isspace);
    scanner_mark_lexeme_start(S);
state_0:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 39: goto state_7;
        case 45: goto state_6;
        case 58: goto state_5;
        case 59: goto state_4;
        case 65: goto state_3;
        case 66: goto state_3;
        case 67: goto state_3;
        case 68: goto state_3;
        case 69: goto state_3;
        case 70: goto state_3;
        case 71: goto state_3;
        case 72: goto state_3;
        case 73: goto state_3;
        case 74: goto state_3;
        case 75: goto state_3;
        case 76: goto state_3;
        case 77: goto state_3;
        case 78: goto state_3;
        case 79: goto state_3;
        case 80: goto state_3;
        case 81: goto state_3;
        case 82: goto state_3;
        case 83: goto state_3;
        case 84: goto state_3;
        case 85: goto state_3;
        case 86: goto state_3;
        case 87: goto state_3;
        case 88: goto state_3;
        case 89: goto state_3;
        case 90: goto state_3;
        case 94: goto state_2;
        case 97: goto state_1;
        case 98: goto state_1;
        case 99: goto state_1;
        case 100: goto state_1;
        case 101: goto state_1;
        case 102: goto state_1;
        case 103: goto state_1;
        case 104: goto state_1;
        case 105: goto state_1;
        case 106: goto state_1;
        case 107: goto state_1;
        case 108: goto state_1;
        case 109: goto state_1;
        case 110: goto state_1;
        case 111: goto state_1;
        case 112: goto state_1;
        case 113: goto state_1;
        case 114: goto state_1;
        case 115: goto state_1;
        case 116: goto state_1;
        case 117: goto state_1;
        case 118: goto state_1;
        case 119: goto state_1;
        case 120: goto state_1;
        case 121: goto state_1;
        case 122: goto state_1;
        default: goto undo_and_commit;
    }
state_1:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = 1;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 109: goto state_1;
        case 110: goto state_1;
        case 111: goto state_1;
        case 112: goto state_1;
        case 113: goto state_1;
        case 114: goto state_1;
        case 115: goto state_1;
        case 116: goto state_1;
        case 117: goto state_1;
        case 118: goto state_1;
        case 119: goto state_1;
        case 120: goto state_1;
        case 121: goto state_1;
        case 122: goto state_1;
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
        case 65: goto state_1;
        case 66: goto state_1;
        case 67: goto state_1;
        case 68: goto state_1;
        case 69: goto state_1;
        case 70: goto state_1;
        case 71: goto state_1;
        case 72: goto state_1;
        case 73: goto state_1;
        case 74: goto state_1;
        case 75: goto state_1;
        case 76: goto state_1;
        case 77: goto state_1;
        case 78: goto state_1;
        case 79: goto state_1;
        case 80: goto state_1;
        case 81: goto state_1;
        case 82: goto state_1;
        case 83: goto state_1;
        case 84: goto state_1;
        case 85: goto state_1;
        case 86: goto state_1;
        case 87: goto state_1;
        case 88: goto state_1;
        case 89: goto state_1;
        case 90: goto state_1;
        case 95: goto state_1;
        case 97: goto state_1;
        case 98: goto state_1;
        case 99: goto state_1;
        case 100: goto state_1;
        case 101: goto state_1;
        case 102: goto state_1;
        case 103: goto state_1;
        case 104: goto state_1;
        case 105: goto state_1;
        case 106: goto state_1;
        case 107: goto state_1;
        case 108: goto state_1;
        default: goto undo_and_commit;
    }
state_2:
    pterm = 7;
    goto commit;
state_3:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = 0;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 48: goto state_3;
        case 49: goto state_3;
        case 50: goto state_3;
        case 51: goto state_3;
        case 52: goto state_3;
        case 53: goto state_3;
        case 54: goto state_3;
        case 55: goto state_3;
        case 56: goto state_3;
        case 57: goto state_3;
        case 65: goto state_3;
        case 66: goto state_3;
        case 67: goto state_3;
        case 68: goto state_3;
        case 69: goto state_3;
        case 70: goto state_3;
        case 71: goto state_3;
        case 72: goto state_3;
        case 73: goto state_3;
        case 74: goto state_3;
        case 75: goto state_3;
        case 76: goto state_3;
        case 77: goto state_3;
        case 78: goto state_3;
        case 79: goto state_3;
        case 80: goto state_3;
        case 81: goto state_3;
        case 82: goto state_3;
        case 83: goto state_3;
        case 84: goto state_3;
        case 85: goto state_3;
        case 86: goto state_3;
        case 87: goto state_3;
        case 88: goto state_3;
        case 89: goto state_3;
        case 90: goto state_3;
        case 95: goto state_3;
        case 97: goto state_3;
        case 98: goto state_3;
        case 99: goto state_3;
        case 100: goto state_3;
        case 101: goto state_3;
        case 102: goto state_3;
        case 103: goto state_3;
        case 104: goto state_3;
        case 105: goto state_3;
        case 106: goto state_3;
        case 107: goto state_3;
        case 108: goto state_3;
        case 109: goto state_3;
        case 110: goto state_3;
        case 111: goto state_3;
        case 112: goto state_3;
        case 113: goto state_3;
        case 114: goto state_3;
        case 115: goto state_3;
        case 116: goto state_3;
        case 117: goto state_3;
        case 118: goto state_3;
        case 119: goto state_3;
        case 120: goto state_3;
        case 121: goto state_3;
        case 122: goto state_3;
        default: goto undo_and_commit;
    }
state_4:
    pterm = 5;
    goto commit;
state_5:
    pterm = 4;
    goto commit;
state_6:
    pterm = 6;
    goto commit;
state_7:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 0: goto state_8;
        case 1: goto state_8;
        case 2: goto state_8;
        case 3: goto state_8;
        case 4: goto state_8;
        case 5: goto state_8;
        case 6: goto state_8;
        case 7: goto state_8;
        case 8: goto state_8;
        case 9: goto state_8;
        case 11: goto state_8;
        case 12: goto state_8;
        case 13: goto state_8;
        case 14: goto state_8;
        case 15: goto state_8;
        case 16: goto state_8;
        case 17: goto state_8;
        case 18: goto state_8;
        case 19: goto state_8;
        case 20: goto state_8;
        case 21: goto state_8;
        case 22: goto state_8;
        case 23: goto state_8;
        case 24: goto state_8;
        case 25: goto state_8;
        case 26: goto state_8;
        case 27: goto state_8;
        case 28: goto state_8;
        case 29: goto state_8;
        case 30: goto state_8;
        case 31: goto state_8;
        case 32: goto state_8;
        case 33: goto state_8;
        case 34: goto state_8;
        case 35: goto state_8;
        case 36: goto state_8;
        case 37: goto state_8;
        case 38: goto state_8;
        case 39: goto state_9;
        case 40: goto state_8;
        case 41: goto state_8;
        case 42: goto state_8;
        case 43: goto state_8;
        case 44: goto state_8;
        case 45: goto state_8;
        case 46: goto state_8;
        case 47: goto state_8;
        case 48: goto state_8;
        case 49: goto state_8;
        case 50: goto state_8;
        case 51: goto state_8;
        case 52: goto state_8;
        case 53: goto state_8;
        case 54: goto state_8;
        case 55: goto state_8;
        case 56: goto state_8;
        case 57: goto state_8;
        case 58: goto state_8;
        case 59: goto state_8;
        case 60: goto state_8;
        case 61: goto state_8;
        case 62: goto state_8;
        case 63: goto state_8;
        case 64: goto state_8;
        case 65: goto state_8;
        case 66: goto state_8;
        case 67: goto state_8;
        case 68: goto state_8;
        case 69: goto state_8;
        case 70: goto state_8;
        case 71: goto state_8;
        case 72: goto state_8;
        case 73: goto state_8;
        case 74: goto state_8;
        case 75: goto state_8;
        case 76: goto state_8;
        case 77: goto state_8;
        case 78: goto state_8;
        case 79: goto state_8;
        case 80: goto state_8;
        case 81: goto state_8;
        case 82: goto state_8;
        case 83: goto state_8;
        case 84: goto state_8;
        case 85: goto state_8;
        case 86: goto state_8;
        case 87: goto state_8;
        case 88: goto state_8;
        case 89: goto state_8;
        case 90: goto state_8;
        case 91: goto state_8;
        case 92: goto state_8;
        case 93: goto state_8;
        case 94: goto state_8;
        case 95: goto state_8;
        case 96: goto state_8;
        case 97: goto state_8;
        case 98: goto state_8;
        case 99: goto state_8;
        case 100: goto state_8;
        case 101: goto state_8;
        case 102: goto state_8;
        case 103: goto state_8;
        case 104: goto state_8;
        case 105: goto state_8;
        case 106: goto state_8;
        case 107: goto state_8;
        case 108: goto state_8;
        case 109: goto state_8;
        case 110: goto state_8;
        case 111: goto state_8;
        case 112: goto state_8;
        case 113: goto state_8;
        case 114: goto state_8;
        case 115: goto state_8;
        case 116: goto state_8;
        case 117: goto state_8;
        case 118: goto state_8;
        case 119: goto state_8;
        case 120: goto state_8;
        case 121: goto state_8;
        case 122: goto state_8;
        case 123: goto state_8;
        case 124: goto state_8;
        case 125: goto state_8;
        case 126: goto state_8;
        case 127: goto state_8;
        default: goto undo_and_commit;
    }
state_8:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    ++nc;
    switch(cc) {
        case 54: goto state_8;
        case 55: goto state_8;
        case 56: goto state_8;
        case 57: goto state_8;
        case 58: goto state_8;
        case 59: goto state_8;
        case 60: goto state_8;
        case 61: goto state_8;
        case 62: goto state_8;
        case 63: goto state_8;
        case 64: goto state_8;
        case 65: goto state_8;
        case 66: goto state_8;
        case 67: goto state_8;
        case 68: goto state_8;
        case 69: goto state_8;
        case 70: goto state_8;
        case 71: goto state_8;
        case 72: goto state_8;
        case 73: goto state_8;
        case 74: goto state_8;
        case 75: goto state_8;
        case 76: goto state_8;
        case 77: goto state_8;
        case 78: goto state_8;
        case 79: goto state_8;
        case 80: goto state_8;
        case 81: goto state_8;
        case 82: goto state_8;
        case 83: goto state_8;
        case 84: goto state_8;
        case 85: goto state_8;
        case 86: goto state_8;
        case 87: goto state_8;
        case 88: goto state_8;
        case 89: goto state_8;
        case 90: goto state_8;
        case 91: goto state_8;
        case 92: goto state_8;
        case 93: goto state_8;
        case 94: goto state_8;
        case 95: goto state_8;
        case 96: goto state_8;
        case 97: goto state_8;
        case 98: goto state_8;
        case 99: goto state_8;
        case 100: goto state_8;
        case 101: goto state_8;
        case 102: goto state_8;
        case 103: goto state_8;
        case 104: goto state_8;
        case 105: goto state_8;
        case 106: goto state_8;
        case 107: goto state_8;
        case 108: goto state_8;
        case 109: goto state_8;
        case 110: goto state_8;
        case 111: goto state_8;
        case 112: goto state_8;
        case 113: goto state_8;
        case 114: goto state_8;
        case 115: goto state_8;
        case 116: goto state_8;
        case 117: goto state_8;
        case 118: goto state_8;
        case 119: goto state_8;
        case 120: goto state_8;
        case 121: goto state_8;
        case 122: goto state_8;
        case 123: goto state_8;
        case 124: goto state_8;
        case 125: goto state_8;
        case 126: goto state_8;
        case 127: goto state_8;
        case 0: goto state_8;
        case 1: goto state_8;
        case 2: goto state_8;
        case 3: goto state_8;
        case 4: goto state_8;
        case 5: goto state_8;
        case 6: goto state_8;
        case 7: goto state_8;
        case 8: goto state_8;
        case 9: goto state_8;
        case 11: goto state_8;
        case 12: goto state_8;
        case 13: goto state_8;
        case 14: goto state_8;
        case 15: goto state_8;
        case 16: goto state_8;
        case 17: goto state_8;
        case 18: goto state_8;
        case 19: goto state_8;
        case 20: goto state_8;
        case 21: goto state_8;
        case 22: goto state_8;
        case 23: goto state_8;
        case 24: goto state_8;
        case 25: goto state_8;
        case 26: goto state_8;
        case 27: goto state_8;
        case 28: goto state_8;
        case 29: goto state_8;
        case 30: goto state_8;
        case 31: goto state_8;
        case 32: goto state_8;
        case 33: goto state_8;
        case 34: goto state_8;
        case 35: goto state_8;
        case 36: goto state_8;
        case 37: goto state_8;
        case 38: goto state_8;
        case 39: goto state_10;
        case 40: goto state_8;
        case 41: goto state_8;
        case 42: goto state_8;
        case 43: goto state_8;
        case 44: goto state_8;
        case 45: goto state_8;
        case 46: goto state_8;
        case 47: goto state_8;
        case 48: goto state_8;
        case 49: goto state_8;
        case 50: goto state_8;
        case 51: goto state_8;
        case 52: goto state_8;
        case 53: goto state_8;
        default: goto undo_and_commit;
    }
state_9:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = 2;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 56: goto state_8;
        case 57: goto state_8;
        case 58: goto state_8;
        case 59: goto state_8;
        case 60: goto state_8;
        case 61: goto state_8;
        case 62: goto state_8;
        case 63: goto state_8;
        case 64: goto state_8;
        case 65: goto state_8;
        case 66: goto state_8;
        case 67: goto state_8;
        case 68: goto state_8;
        case 69: goto state_8;
        case 70: goto state_8;
        case 71: goto state_8;
        case 72: goto state_8;
        case 73: goto state_8;
        case 74: goto state_8;
        case 75: goto state_8;
        case 76: goto state_8;
        case 77: goto state_8;
        case 78: goto state_8;
        case 79: goto state_8;
        case 80: goto state_8;
        case 81: goto state_8;
        case 82: goto state_8;
        case 83: goto state_8;
        case 84: goto state_8;
        case 85: goto state_8;
        case 86: goto state_8;
        case 87: goto state_8;
        case 88: goto state_8;
        case 89: goto state_8;
        case 90: goto state_8;
        case 91: goto state_8;
        case 92: goto state_8;
        case 93: goto state_8;
        case 94: goto state_8;
        case 95: goto state_8;
        case 96: goto state_8;
        case 97: goto state_8;
        case 98: goto state_8;
        case 99: goto state_8;
        case 100: goto state_8;
        case 101: goto state_8;
        case 102: goto state_8;
        case 103: goto state_8;
        case 104: goto state_8;
        case 105: goto state_8;
        case 106: goto state_8;
        case 107: goto state_8;
        case 108: goto state_8;
        case 109: goto state_8;
        case 110: goto state_8;
        case 111: goto state_8;
        case 112: goto state_8;
        case 113: goto state_8;
        case 114: goto state_8;
        case 115: goto state_8;
        case 116: goto state_8;
        case 117: goto state_8;
        case 118: goto state_8;
        case 119: goto state_8;
        case 120: goto state_8;
        case 121: goto state_8;
        case 122: goto state_8;
        case 123: goto state_8;
        case 124: goto state_8;
        case 125: goto state_8;
        case 126: goto state_8;
        case 127: goto state_8;
        case 0: goto state_8;
        case 1: goto state_8;
        case 2: goto state_8;
        case 3: goto state_8;
        case 4: goto state_8;
        case 5: goto state_8;
        case 6: goto state_8;
        case 7: goto state_8;
        case 8: goto state_8;
        case 9: goto state_8;
        case 11: goto state_8;
        case 12: goto state_8;
        case 13: goto state_8;
        case 14: goto state_8;
        case 15: goto state_8;
        case 16: goto state_8;
        case 17: goto state_8;
        case 18: goto state_8;
        case 19: goto state_8;
        case 20: goto state_8;
        case 21: goto state_8;
        case 22: goto state_8;
        case 23: goto state_8;
        case 24: goto state_8;
        case 25: goto state_8;
        case 26: goto state_8;
        case 27: goto state_8;
        case 28: goto state_8;
        case 29: goto state_8;
        case 30: goto state_8;
        case 31: goto state_8;
        case 32: goto state_8;
        case 33: goto state_8;
        case 34: goto state_8;
        case 35: goto state_8;
        case 36: goto state_8;
        case 37: goto state_8;
        case 38: goto state_8;
        case 39: goto state_10;
        case 40: goto state_8;
        case 41: goto state_8;
        case 42: goto state_8;
        case 43: goto state_8;
        case 44: goto state_8;
        case 45: goto state_8;
        case 46: goto state_8;
        case 47: goto state_8;
        case 48: goto state_8;
        case 49: goto state_8;
        case 50: goto state_8;
        case 51: goto state_8;
        case 52: goto state_8;
        case 53: goto state_8;
        case 54: goto state_8;
        case 55: goto state_8;
        default: goto undo_and_commit;
    }
state_10:
    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }
    pterm = term;
    term = 3;
    pnc = nc;
    seen_accepting_state = 1;
    ++nc;
    switch(cc) {
        case 0: goto state_8;
        case 1: goto state_8;
        case 2: goto state_8;
        case 3: goto state_8;
        case 4: goto state_8;
        case 5: goto state_8;
        case 6: goto state_8;
        case 7: goto state_8;
        case 8: goto state_8;
        case 9: goto state_8;
        case 11: goto state_8;
        case 12: goto state_8;
        case 13: goto state_8;
        case 14: goto state_8;
        case 15: goto state_8;
        case 16: goto state_8;
        case 17: goto state_8;
        case 18: goto state_8;
        case 19: goto state_8;
        case 20: goto state_8;
        case 21: goto state_8;
        case 22: goto state_8;
        case 23: goto state_8;
        case 24: goto state_8;
        case 25: goto state_8;
        case 26: goto state_8;
        case 27: goto state_8;
        case 28: goto state_8;
        case 29: goto state_8;
        case 30: goto state_8;
        case 31: goto state_8;
        case 32: goto state_8;
        case 33: goto state_8;
        case 34: goto state_8;
        case 35: goto state_8;
        case 36: goto state_8;
        case 37: goto state_8;
        case 38: goto state_8;
        case 39: goto state_10;
        case 40: goto state_8;
        case 41: goto state_8;
        case 42: goto state_8;
        case 43: goto state_8;
        case 44: goto state_8;
        case 45: goto state_8;
        case 46: goto state_8;
        case 47: goto state_8;
        case 48: goto state_8;
        case 49: goto state_8;
        case 50: goto state_8;
        case 51: goto state_8;
        case 52: goto state_8;
        case 53: goto state_8;
        case 54: goto state_8;
        case 55: goto state_8;
        case 56: goto state_8;
        case 57: goto state_8;
        case 58: goto state_8;
        case 59: goto state_8;
        case 60: goto state_8;
        case 61: goto state_8;
        case 62: goto state_8;
        case 63: goto state_8;
        case 64: goto state_8;
        case 65: goto state_8;
        case 66: goto state_8;
        case 67: goto state_8;
        case 68: goto state_8;
        case 69: goto state_8;
        case 70: goto state_8;
        case 71: goto state_8;
        case 72: goto state_8;
        case 73: goto state_8;
        case 74: goto state_8;
        case 75: goto state_8;
        case 76: goto state_8;
        case 77: goto state_8;
        case 78: goto state_8;
        case 79: goto state_8;
        case 80: goto state_8;
        case 81: goto state_8;
        case 82: goto state_8;
        case 83: goto state_8;
        case 84: goto state_8;
        case 85: goto state_8;
        case 86: goto state_8;
        case 87: goto state_8;
        case 88: goto state_8;
        case 89: goto state_8;
        case 90: goto state_8;
        case 91: goto state_8;
        case 92: goto state_8;
        case 93: goto state_8;
        case 94: goto state_8;
        case 95: goto state_8;
        case 96: goto state_8;
        case 97: goto state_8;
        case 98: goto state_8;
        case 99: goto state_8;
        case 100: goto state_8;
        case 101: goto state_8;
        case 102: goto state_8;
        case 103: goto state_8;
        case 104: goto state_8;
        case 105: goto state_8;
        case 106: goto state_8;
        case 107: goto state_8;
        case 108: goto state_8;
        case 109: goto state_8;
        case 110: goto state_8;
        case 111: goto state_8;
        case 112: goto state_8;
        case 113: goto state_8;
        case 114: goto state_8;
        case 115: goto state_8;
        case 116: goto state_8;
        case 117: goto state_8;
        case 118: goto state_8;
        case 119: goto state_8;
        case 120: goto state_8;
        case 121: goto state_8;
        case 122: goto state_8;
        case 123: goto state_8;
        case 124: goto state_8;
        case 125: goto state_8;
        case 126: goto state_8;
        case 127: goto state_8;
        default: goto undo_and_commit;
    }
undo_and_commit:
    if(!seen_accepting_state) {
        std_error("Scanner Error: Unable to match token.");
    }
    scanner_pushback(S, nc - pnc);
commit:
    scanner_mark_lexeme_end(S);
    if(seen_accepting_state && pterm < 0 && term >= 0) {
        return term;
    }
    return pterm;
}

#endif

