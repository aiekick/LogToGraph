#pragma once

// Pure text-analysis helpers behind the code editor (token extraction, call-target
// resolution, signature arg-index scan, completion prefix filter). No ImGui / ImCode
// dependency on purpose: these are exercised headless by the app's unit tests.
//
// All columns are BYTE offsets into the line (ImCode's coordinate system). The old
// TextEditor reported visual (tab-expanded) columns, which forced every consumer to
// re-walk the line; that conversion died with the ImCode port.

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace ltg {
namespace code {

bool isIdentChar(char aChar);

// Identifier covering the byte at aByteColumn of aLineText.
// Returns "" when the byte is whitespace/punctuation, out of range, or when the
// covered word is a pure number literal (identifiers must not start with a digit).
std::string extractTokenAt(const std::string& aLineText, int32_t aByteColumn);

// Resolves `target?.functionName` from the bytes immediately to the LEFT of the `(`
// at byte index aParenByteColumn. Accepts NO whitespace between the function name and
// the `(`, and only a single `.` or `:` separator before an optional target.
// Returns false when there is no identifier right before the `(`.
bool extractCallTargetAt(const std::string& aLineText, int32_t aParenByteColumn, std::string& aoTarget, std::string& aoFunctionName);

// Walks aLineText[aAnchorByte, aCursorByte) — the span between the opening `(` and the
// caret — tracking paren depth and comma count at depth 0. String literals ('...'/"...")
// are skipped so a `,` or `(` inside them doesn't disturb the count.
// Returns false when the caret has passed the matching `)` (the call is finished and the
// signature tooltip should close); otherwise true with aoArgIndex = current arg index.
bool computeSignatureArgIndex(const std::string& aLineText, size_t aAnchorByte, size_t aCursorByte, int32_t& aoArgIndex);

// Case-sensitive prefix filter: returns the indexes (in order) of aNames entries
// starting with aPrefix. An empty prefix matches everything.
std::vector<size_t> filterByPrefix(const std::vector<std::string>& aNames, const std::string& aPrefix);

}  // namespace code
}  // namespace ltg
