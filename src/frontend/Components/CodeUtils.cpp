#include "CodeUtils.h"

namespace ltg {
namespace code {

bool isIdentChar(char aChar) {
    return (aChar >= 'A' && aChar <= 'Z') || (aChar >= 'a' && aChar <= 'z') || (aChar >= '0' && aChar <= '9') || aChar == '_';
}

std::string extractTokenAt(const std::string& aLineText, int32_t aByteColumn) {
    if (aByteColumn < 0 || (size_t)aByteColumn >= aLineText.size()) {
        return std::string();
    }
    if (!isIdentChar(aLineText[(size_t)aByteColumn])) {
        return std::string();
    }
    size_t leftBound = (size_t)aByteColumn;
    while (leftBound > 0 && isIdentChar(aLineText[leftBound - 1])) {
        --leftBound;
    }
    size_t rightBound = (size_t)aByteColumn;
    while (rightBound < aLineText.size() && isIdentChar(aLineText[rightBound])) {
        ++rightBound;
    }
    if (rightBound <= leftBound) {
        return std::string();
    }
    // reject pure number literals (identifier rule: must not start with a digit)
    const char firstChar = aLineText[leftBound];
    if (firstChar >= '0' && firstChar <= '9') {
        return std::string();
    }
    return aLineText.substr(leftBound, rightBound - leftBound);
}

bool extractCallTargetAt(const std::string& aLineText, int32_t aParenByteColumn, std::string& aoTarget, std::string& aoFunctionName) {
    aoTarget.clear();
    aoFunctionName.clear();
    if (aParenByteColumn <= 0 || (size_t)aParenByteColumn > aLineText.size()) {
        return false;  // nothing to the left of the `(`
    }
    const size_t parenByte = (size_t)aParenByteColumn;

    // the function name ends RIGHT before the `(` — bail out on whitespace / punctuation
    if (!isIdentChar(aLineText[parenByte - 1])) {
        return false;
    }
    size_t funcStart = parenByte - 1;
    while (funcStart > 0 && isIdentChar(aLineText[funcStart - 1])) {
        --funcStart;
    }
    // reject pure number literals (identifier rule: must not start with a digit)
    if (aLineText[funcStart] >= '0' && aLineText[funcStart] <= '9') {
        return false;
    }
    aoFunctionName = aLineText.substr(funcStart, parenByte - funcStart);

    // optional `target.` or `target:` separator right before the function name
    if (funcStart == 0) {
        return true;
    }
    const char sep = aLineText[funcStart - 1];
    if (sep != ':' && sep != '.') {
        return true;
    }
    if (funcStart < 2 || !isIdentChar(aLineText[funcStart - 2])) {
        return true;  // separator with nothing valid before — leave target empty
    }
    size_t targetEnd = funcStart - 1;  // exclusive boundary (the separator)
    size_t targetStart = targetEnd - 1;
    while (targetStart > 0 && isIdentChar(aLineText[targetStart - 1])) {
        --targetStart;
    }
    if (aLineText[targetStart] >= '0' && aLineText[targetStart] <= '9') {
        return true;  // bad target — leave empty
    }
    aoTarget = aLineText.substr(targetStart, targetEnd - targetStart);
    return true;
}

bool computeSignatureArgIndex(const std::string& aLineText, size_t aAnchorByte, size_t aCursorByte, int32_t& aoArgIndex) {
    aoArgIndex = 0;
    int32_t depth = 0;
    int32_t commaCount = 0;
    bool inString = false;
    char stringDelim = 0;
    for (size_t i = aAnchorByte; i < aCursorByte && i < aLineText.size(); ++i) {
        const char c = aLineText[i];
        if (inString) {
            if (c == '\\' && i + 1 < aLineText.size()) {
                ++i;  // skip escaped char
                continue;
            }
            if (c == stringDelim) {
                inString = false;
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            inString = true;
            stringDelim = c;
            continue;
        }
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
            if (depth < 0) {
                return false;  // caret moved past the matching `)` — call is finished
            }
        } else if (c == ',' && depth == 0) {
            ++commaCount;
        }
    }
    aoArgIndex = commaCount;
    return true;
}

std::vector<size_t> filterByPrefix(const std::vector<std::string>& aNames, const std::string& aPrefix) {
    std::vector<size_t> indexes;
    indexes.reserve(aNames.size());
    for (size_t i = 0; i < aNames.size(); ++i) {
        const std::string& name = aNames[i];
        if (name.size() >= aPrefix.size() && name.compare(0, aPrefix.size(), aPrefix) == 0) {
            indexes.push_back(i);
        }
    }
    return indexes;
}

}  // namespace code
}  // namespace ltg
