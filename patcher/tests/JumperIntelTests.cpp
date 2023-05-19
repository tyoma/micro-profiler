#include <patcher/jumper.h>

#include "helpers.h"

#include <common/memory.h>
#include <iterator>
#include <patcher/exceptions.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct auto_jumper : jumper, noncopyable
			{
				auto_jumper(void *target, const void *divert_to)
					: jumper(target, divert_to)
				{	}

				~auto_jumper()
				{
					mem_set(prologue(), 0, prologue_size());
					revert();
				}
			};
		}

		string one();
		string two();
		string three();	// expected not to be modified
		string four();	// expected not to be modified

		begin_test_suite( JumperIntelTests )
			shared_ptr<byte> edge;
			shared_ptr<void> scope;

			init( Init )
			{
				edge = static_pointer_cast<byte>(allocate_edge());
				scope = temporary_unlock_code_at(address_cast_hack<void *>(&one));
			}


			test( ConstructionSucceedsWhenAJumperLengthImmediatelyPrecedingTargetAreAvailableForNOPsAtEntry )
			{
				// INIT
				auto target = edge.get() + c_jumper_size;

				// INIT / ACT / ASSERT (must not throw)
				target[0] = 0x90, target[1] = 0x90; // nop, nop
				auto_jumper(target, 0);
				assert_equal(target + 2, static_cast<const byte *>(auto_jumper(target, 0).entry()));
				assert_is_false(auto_jumper(target, 0).active());
				target[0] = 0x66, target[1] = 0x90; // xchg ax, ax
				auto_jumper (target, 0);
				assert_equal(target + 2, static_cast<const byte *>(auto_jumper(target, 0).entry()));
				target[0] = 0x8B, target[1] = 0xFF; // mov edi, edi
				auto_jumper(target, 0);
				assert_equal(target + 2, static_cast<const byte *>(auto_jumper(target, 0).entry()));
				target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x00; // 3-byte nop
				auto_jumper(target, 0);
				assert_equal(target + 3, static_cast<const byte *>(auto_jumper(target, 0).entry()));
				target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x40, target[3] = 0x08; // 4-byte nop
				auto_jumper(target, 0);
				assert_equal(target + 4, static_cast<const byte *>(auto_jumper(target, 0).entry()));
			}

//#ifdef WIN32	// It is possible to prepare the edge allocation validly only on Windows
//			test( ConstructionFailsIfPrecedeedAllocatedBytesAreInsufficientForANOPJumper )
//			{
//				// INIT
//				auto target = edge.get() + c_jumper_size - 1 /*one byte short of allocated space*/;
//
//				// INIT / ACT / ASSERT
//				target[0] = 0x90, target[1] = 0x90; // nop, nop
//				assert_throws(auto_jumper(target, 0), padding_insufficient);
//				target[0] = 0x66, target[1] = 0x90; // xchg ax, ax
//				assert_throws(auto_jumper(target, 0), padding_insufficient);
//				target[0] = 0x8B, target[1] = 0xFF; // mov edi, edi
//				assert_throws(auto_jumper(target, 0), padding_insufficient);
//				target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x00; // 3-byte nop
//				assert_throws(auto_jumper(target, 0), padding_insufficient);
//				target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x40, target[3] = 0x08; // 4-byte nop
//				assert_throws(auto_jumper(target, 0), padding_insufficient);
//			}
//#endif


			test( MorePaddingIsRequiredForRealMultibyteInstructions )
			{
				// INIT
				const auto jmp_size = 2; // change to sizeof short_jump

				const auto ok = edge.get() + c_jumper_size + jmp_size;

				// INIT
				auto ok_target = ok + 2;
				ok_target[0] = 0xF7, ok_target[1] = 0xF9; // idiv ecx

				// ACT / ASSERT
				auto_jumper(ok_target, 0);
				assert_equal(ok_target - 4, auto_jumper(ok_target, 0).entry());

				// INIT
				ok_target = ok + 3;
				ok_target[-1] = 0x00,
				ok_target[0] = 0x89, ok_target[1] = 0x65, ok_target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]

				// ACT / ASSERT
				auto_jumper(ok_target, 0);
				assert_equal(ok_target - 5, auto_jumper(ok_target, 0).entry());

				// INIT
				ok_target = ok + 4;
				ok_target[-1] = 0x00,
				ok_target[0] = 0x8D, ok_target[1] = 0x44, ok_target[2] = 0x30; // lea dword ptr [rax+rsi*1-0x1], eax
				ok_target[3] = 0xFF;

				// ACT / ASSERT
				auto_jumper(ok_target, 0);
				assert_equal(ok_target - 6, auto_jumper(ok_target, 0).entry());

				// INIT
				ok_target = ok + 5;
				ok_target[-1] = 0x00,
				ok_target[0] = 0xB8, ok_target[1] = 0x00, ok_target[2] = 0x00; // mov eax, 00000000h
				ok_target[3] = 0x00, ok_target[4] = 0x00;

				// ACT / ASSERT
				auto_jumper(ok_target, 0);

				// INIT
				ok_target = ok + 6;
				ok_target[-1] = 0x00,
				ok_target[0] = 0x8B, ok_target[1] = 0x8D, ok_target[2] = 0x54; // mov dword ptr [rbp-0x2ac], ecx
				ok_target[3] = 0xFD, ok_target[4] = 0xFF, ok_target[5] = 0xFF;

				// ACT / ASSERT
				auto_jumper(ok_target, 0);
			}


			test( JumperFailsToWrapAFunctionWithADirtyPrologueNOP )
			{
				// INIT
				auto target = edge.get() + c_jumper_size;

				// INIT / ACT / ASSERT
				target[0] = 0x66, target[1] = 0x90; // xchg ax, ax
				target[-(c_jumper_size / 2)] = 0x12;
				assert_throws(auto_jumper(target, 0), padding_insufficient);
				fill(target - c_jumper_size, target, (byte)0xCC);
				fill(target - c_jumper_size / 3, target, (byte)0x90);
				assert_throws(auto_jumper(target, 0), padding_insufficient);
				fill(target - c_jumper_size, target, (byte)0x90);
				fill(target - 2 * c_jumper_size / 3, target - c_jumper_size / 3, (byte)0x9B);
				assert_throws(auto_jumper(target, 0), padding_insufficient);
				fill(target - c_jumper_size, target, (byte)0x90);
				target[-c_jumper_size] = 0x12;
				assert_throws(auto_jumper(target, 0), padding_insufficient);
			}


			test( JumperFailsToWrapAFunctionWithADirtyPrologueOpcode )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 6;

				target[0] = 0xF7, target[1] = 0xF9; // idiv ecx
				target[-(c_jumper_size + 4)] = 0x12;

				// INIT / ACT / ASSERT
				assert_throws(auto_jumper(target, 0), padding_insufficient);

				// INIT
				fill(target - (c_jumper_size + 4), target, (byte)0xCC);
				target[-1] = 0x12;

				// INIT / ACT / ASSERT
				assert_throws(auto_jumper(target, 0), padding_insufficient);

				// INIT
				fill(target - (c_jumper_size + 5), target, (byte)0x90);
				target[-(c_jumper_size + 5)] = 0x87;
				target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]

				// INIT / ACT / ASSERT
				assert_throws(auto_jumper(target, 0), padding_insufficient);

				// INIT
				fill(target - (c_jumper_size + 5), target, (byte)0x83);
				target[-1] = 0x12;

				// INIT / ACT / ASSERT
				assert_throws(auto_jumper(target, 0), padding_insufficient);
			}


			test( FunctionsWorkNormallyAfterJumperConstruction )
			{
				// INIT / ACT
				jumper j1(address_cast_hack<void *>(&one), 0);
				jumper j2(address_cast_hack<void *>(&two), 0);

				// ACT / ASSERT
				assert_equal("one", one());
				assert_equal("two", two());
			}


			test( JumperIsInitializedOnConstruction )
			{
				// INIT / ACT
				auto target = edge.get() + c_jumper_size;
				fill(target - c_jumper_size, target, (byte)0xC9);
				target[0] = 0x66, target[1] = 0x90; // xchg ax, ax

				{
					jumper j(target, address_cast_hack<void *>(&three));

				// ACT / ASSERT
					assert_equal("three", address_cast_hack<string (*)()>(target - c_jumper_size)());
				}

				// INIT / ACT
				target = edge.get() + c_jumper_size + 5;
				fill(target - c_jumper_size - 5, target, (byte)0xC9);
				target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]

				{
					jumper j(target, address_cast_hack<void *>(&four));

				// ACT / ASSERT
					assert_equal("four", address_cast_hack<string (*)()>(target - c_jumper_size - 5)());
				}
			}


			test( EntryIsCallableOnConstruction )
			{
				// INIT / ACT
				jumper j1(address_cast_hack<void *>(&one), 0);
				jumper j2(address_cast_hack<void *>(&two), 0);

				// ACT / ASSERT
				assert_equal("one", address_cast_hack<string (*)()>(j1.entry())());
				assert_equal("two", address_cast_hack<string (*)()>(j2.entry())());

				// ASSERT
				assert_equal(address_cast_hack<void *>(&one), j1.target());
				assert_equal(address_cast_hack<void *>(&two), j2.target());
			}


			test( CallsAreDivertedAfterJumperActivation )
			{
				// INIT / ACT
				jumper j1(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));
				jumper j2(address_cast_hack<void *>(&two), address_cast_hack<void *>(&four));

				// ACT / ASSERT
				assert_is_true(j1.activate());
				assert_is_true(j2.activate());

				// ACT / ASSERT
				assert_equal("three", one());
				assert_equal("four", two());

				// ASSERT
				assert_is_true(j1.active());
				assert_is_true(j2.active());
			}


			test( DiversionEndsWhenJumperIsReverted )
			{
				// INIT / ACT
				jumper j1(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));
				jumper j2(address_cast_hack<void *>(&two), address_cast_hack<void *>(&four));

				j1.activate();
				j2.activate();

				// ACT
				assert_is_true(j1.revert());
				assert_is_true(j2.revert());

				// ACT / ASSERT
				assert_equal("one", one());
				assert_equal("two", two());

				// ASSERT
				assert_is_false(j1.active());
				assert_is_false(j2.active());
			}


			test( ConsequentApplicationDoesNothing )
			{
				// INIT
				jumper j(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));

				j.activate();

				// ACT / ASSERT
				assert_is_false(j.activate());
			}


			test( RevertingInactiveJumperDoesNothing )
			{
				// INIT
				jumper j(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));

				// ACT / ASSERT
				assert_is_false(j.revert());

				// ACT / ASSERT
				assert_equal("one", one());
			}


			test( AttemptToConstructOverSingleByteOpcodedFunctionFails )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 4 /*guarantee no problems with space to move to*/;

				// INIT / ACT / ASSERT
				target[0] = 0x52, target[1] = 0xC3; // push ..., ret
				assert_throws(jumper(target, 0), leading_too_short);
				target[0] = 0xC3, target[1] = 0x52; // ret, push ...
				assert_throws(jumper(target, 0), leading_too_short);
			}

		end_test_suite
	}
}
