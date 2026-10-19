"""Protect literal contents independently of the formatter's C++ parser."""
import importlib.util
from pathlib import Path
import re
import unittest

spec = importlib.util.spec_from_file_location(
    "astyle_347", Path(__file__).with_name("astyle_347.py"))
formatter_tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(formatter_tool)


class RawLiteralTest(unittest.TestCase):
    def test_named_empty_and_encoded_literals_preserve_contents(self):
        body = '\n\t["raw", "\\\\path", "café"]\n ) is not a closing delimiter\n'
        for prefix in ("", "u8", "u", "U", "L"):
            for delimiter in ("", "json", "sixteen_chars_01"):
                with self.subTest(prefix=prefix, delimiter=delimiter):
                    literal = f'{prefix}R"{delimiter}({body}){delimiter}"'
                    source = f"    auto value = {literal};\n"
                    result = formatter_tool.format_preserving_raw_literals(
                        source, lambda text: "\n".join(line.strip() for line in text.split("\n")))
                    self.assertIn(literal, result)
                    self.assertTrue(result.startswith("auto value = "))

    def test_comments_quoted_strings_chars_and_digit_separators_are_not_raw_tokens(self):
        literal = 'R"real(\n    keep this\n)real"'
        source = (
            '// R"(unfinished comment\n'
            '/* R"tag(block comment)tag" */\n'
            'auto quoted = "R\\"(ordinary quoted text)";\n'
            "auto quote = '\"'; auto wide = L'R';\n"
            "// continued comment \\\n"
            'R"(still a comment\n'
            f"auto value = 1'000'000 + {literal} + 'x';\n"
        )
        observed = []

        def observe(text):
            observed.append(text)
            return text

        self.assertEqual(
            formatter_tool.format_preserving_raw_literals(source, observe), source)
        self.assertNotIn(literal, observed[0])
        self.assertIn('// R"(unfinished comment', observed[0])
        self.assertIn('R"(still a comment', observed[0])
        self.assertEqual(len(re.findall(r'"__BN_RAW_LITERAL_\d+__"', observed[0])), 1)

    def test_marker_collisions_do_not_change_original_text(self):
        source = 'auto a = "__BN_RAW_LITERAL_0__"; auto b = R"(__BN_RAW_LITERAL__0__)";'
        self.assertEqual(
            formatter_tool.format_preserving_raw_literals(source, lambda text: text), source)

    def test_missing_marker_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "protected raw-string marker"):
            formatter_tool.format_preserving_raw_literals('auto a = R"(value)";', lambda text: "")

    def test_duplicated_marker_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "protected raw-string marker"):
            formatter_tool.format_preserving_raw_literals(
                'auto a = R"(value)";', lambda text: text + text)


if __name__ == "__main__":
    unittest.main()
