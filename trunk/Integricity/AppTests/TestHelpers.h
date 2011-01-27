#pragma once

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }