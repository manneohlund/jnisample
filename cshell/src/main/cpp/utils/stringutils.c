//
// Created by Manne Öhlund on 2019-04-11.
//

#ifndef _STRINGUTILS_
#define _STRINGUTILS_

const char dot = '.';

#define ENDS_WITH_OR_CONTAINS(a,b)                      (strcomp(0, a, b))

#define ENDS_WITH_OR_CONTAINS_CASE_INSENSITIVE(a,b)     (strcomp(1, a, b))

#define CONTAINS(a,b)                                   (strstr(a, b))

#define CONTAINS_CASE_INSENSITIVE(a,b)                  (strcasestr(a, b))

#define ENDS_WITH(a,b)                                  (strend(a, b))

int strend(const char *s, const char *t) {
    int diff = (int) (strlen(s) - strlen(t));
    return diff > 0 && 0 == strcmp(&s[diff], t);
}

int strcomp(const int case_sesitive, const char *s, const char *t) {
    if (t != NULL && t[0] == dot) {
        return ENDS_WITH(s, t);
    }
    return case_sesitive ? CONTAINS_CASE_INSENSITIVE(s, t) != NULL : CONTAINS(s, t) != NULL;
}

#endif