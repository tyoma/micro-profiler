#include <patcher/jumper.h>

#include <patcher/exceptions.h>

#include <common/memory.h>
#include <iterator>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

extern "C" {
	extern micro_profiler::byte c_jumper_size;
}

namespace micro_profiler
{
	namespace tests
	{
		std::string one();
		std::string two();
		std::string three();	// expected not to be modified
		std::string four();	// expected not to be modified

		begin_test_suite( JumperIntelTests )
			shared_ptr<byte> edge;

			init( Init )
			{
				edge = static_pointer_cast<byte>(allocate_edge());
			}


			test( ConstructionSucceedsWhenAJumperLengthImmediatelyPrecedingTargetAreAvailableForNOPsAtEntry )
			{
				// INIT
				auto target = edge.get() + c_jumper_size;

				// INIT / ACT / ASSERT (must not throw)
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x90, target[1] = 0x90;	} // nop, nop
				jumper(target, 0);
				assert_equal(target + 2, static_cast<const byte *>(jumper(target, 0).entry()));
				assert_is_false(jumper(target, 0).active());
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x66, target[1] = 0x90;	} // xchg ax, ax
				jumper (target, 0);
				assert_equal(target + 2, static_cast<const byte *>(jumper(target, 0).entry()));
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x8B, target[1] = 0xFF;	} // mov edi, edi
				jumper(target, 0);
				assert_equal(target + 2, static_cast<const byte *>(jumper(target, 0).entry()));
				{	scoped_unprotect u(byte_range(target, 3)); target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x00;	} // 3-byte nop
				jumper(target, 0);
				assert_equal(target + 3, static_cast<const byte *>(jumper(target, 0).entry()));
				{	scoped_unprotect u(byte_range(target, 4)); target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x40, target[3] = 0x08;	} // 4-byte nop
				jumper(target, 0);
				assert_equal(target + 4, static_cast<const byte *>(jumper(target, 0).entry()));
			}

#ifdef WIN32	// It is possible to prepare the edge allocation validly only on Windows
			test( ConstructionFailsIfPrecedeedAllocatedBytesAreInsufficientForANOPJumper )
			{
				// INIT
				auto target = edge.get() + c_jumper_size - 1 /*one byte short of allocated space*/;

				// INIT / ACT / ASSERT
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x90, target[1] = 0x90;	} // nop, nop
				assert_throws(jumper(target, 0), padding_insufficient);
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x66, target[1] = 0x90;	} // xchg ax, ax
				assert_throws(jumper(target, 0), padding_insufficient);
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x8B, target[1] = 0xFF;	} // mov edi, edi
				assert_throws(jumper(target, 0), padding_insufficient);
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x00;	} // 3-byte nop
				assert_throws(jumper(target, 0), padding_insufficient);
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x0f, target[1] = 0x1f, target[2] = 0x40, target[3] = 0x08;	} // 4-byte nop
				assert_throws(jumper(target, 0), padding_insufficient);
			}
#endif


			test( MorePaddingIsRequiredForRealMultibyteInstructions )
			{
				// INIT
				const auto jmp_size = 2; // change to sizeof short_jump

				const auto edge2 = static_pointer_cast<byte>(allocate_edge());
				const auto ok = edge.get() + c_jumper_size + jmp_size;
				const auto fail = edge2.get() + c_jumper_size + jmp_size - 1;

				// INIT
				auto ok_target = ok + 2;
				auto fail_target = fail + 2;
				{
					scoped_unprotect u1(byte_range(fail_target, 2)), u2(byte_range(ok_target, 2));
					fail_target[0] = ok_target[0] = 0xF7, fail_target[1] = ok_target[1] = 0xF9; // idiv ecx
				}

				// ACT / ASSERT
				jumper(ok_target, 0);
				assert_equal(ok_target - 4, jumper(ok_target, 0).entry());
				assert_throws(jumper(fail_target, 0), padding_insufficient);

				// INIT
				ok_target = ok + 3;
				fail_target = fail + 3;
				{
					scoped_unprotect u1(byte_range(fail_target - 1, 4)), u2(byte_range(ok_target - 1, 4));
					fail_target[-1] = ok_target[-1] = 0x00,
					fail_target[0] = ok_target[0] = 0x89, fail_target[1] = ok_target[1] = 0x65, fail_target[2] = ok_target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}

				// ACT / ASSERT
				jumper(ok_target, 0);
				assert_equal(ok_target - 5, jumper(ok_target, 0).entry());
				assert_throws(jumper(fail_target, 0), padding_insufficient);

				// INIT
				ok_target = ok + 4;
				fail_target = fail + 4;
				{
					scoped_unprotect u1(byte_range(fail_target - 1, 5)), u2(byte_range(ok_target - 1, 5));
					fail_target[-1] = ok_target[-1] = 0x00,
					fail_target[0] = ok_target[0] = 0x8D, fail_target[1] = ok_target[1] = 0x44, fail_target[2] = ok_target[2] = 0x30; // lea dword ptr [rax+rsi*1-0x1], eax
					fail_target[3] = ok_target[3] = 0xFF;
				}

				// ACT / ASSERT
				jumper(ok_target, 0);
				assert_equal(ok_target - 6, jumper(ok_target, 0).entry());
				assert_throws(jumper(fail_target, 0), padding_insufficient);

				// INIT
				ok_target = ok + 5;
				fail_target = fail + 5;
				{
					scoped_unprotect u1(byte_range(fail_target - 1, 6)), u2(byte_range(ok_target - 1, 6));
					fail_target[-1] = ok_target[-1] = 0x00,
					fail_target[0] = ok_target[0] = 0xB8, fail_target[1] = ok_target[1] = 0x00, fail_target[2] = ok_target[2] = 0x00; // mov eax, 00000000h
					fail_target[3] = ok_target[3] = 0x00, fail_target[4] = ok_target[4] = 0x00;
				}

				// ACT / ASSERT
				jumper(ok_target, 0);
				assert_throws(jumper(fail_target, 0), padding_insufficient);

				// INIT
				ok_target = ok + 6;
				fail_target = fail + 6;
				{
					scoped_unprotect u1(byte_range(fail_target - 1, 7)), u2(byte_range(ok_target - 1, 7));
					fail_target[-1] = ok_target[-1] = 0x00,
					fail_target[0] = ok_target[0] = 0x8B, fail_target[1] = ok_target[1] = 0x8D, fail_target[2] = ok_target[2] = 0x54; // mov dword ptr [rbp-0x2ac], ecx
					fail_target[3] = ok_target[3] = 0xFD, fail_target[4] = ok_target[4] = 0xFF, fail_target[5] = ok_target[5] = 0xFF;
				}

				// ACT / ASSERT
				jumper(ok_target, 0);
				assert_throws(jumper(fail_target, 0), padding_insufficient);
			}


			test( JumperFailsToWrapAFunctionWithADirtyPrologueNOP )
			{
				// INIT
				auto target = edge.get() + c_jumper_size;

				// INIT / ACT / ASSERT
				{
					scoped_unprotect u(byte_range(target - c_jumper_size, c_jumper_size + 2));
					target[0] = 0x66, target[1] = 0x90; // xchg ax, ax
					target[-(c_jumper_size / 2)] = 0x12;
				}
				assert_throws(jumper(target, 0), padding_insufficient);
				{
					scoped_unprotect u(byte_range(target - c_jumper_size, c_jumper_size));
					fill(target - c_jumper_size, target, (byte)0xCC);
					fill(target - c_jumper_size / 3, target, (byte)0x90);
				}
				assert_throws(jumper(target, 0), padding_insufficient);
				{
					scoped_unprotect u(byte_range(target - c_jumper_size, c_jumper_size));
					fill(target - c_jumper_size, target, (byte)0x90);
					fill(target - 2 * c_jumper_size / 3, target - c_jumper_size / 3, (byte)0x9B);
				}
				assert_throws(jumper(target, 0), padding_insufficient);
				{
					scoped_unprotect u(byte_range(target - c_jumper_size, c_jumper_size));
					fill(target - c_jumper_size, target, (byte)0x90);
					target[-c_jumper_size] = 0x12;
				}
				assert_throws(jumper(target, 0), padding_insufficient);
			}


			test( JumperFailsToWrapAFunctionWithADirtyPrologueOpcode )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 6;

				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 4, c_jumper_size + 6));
					target[0] = 0xF7, target[1] = 0xF9; // idiv ecx
					target[-(c_jumper_size + 4)] = 0x12;
				}

				// INIT / ACT / ASSERT
				assert_throws(jumper(target, 0), padding_insufficient);

				// INIT
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 4, c_jumper_size + 6));
					fill(target - (c_jumper_size + 4), target, (byte)0xCC);
					target[-1] = 0x12;
				}

				// INIT / ACT / ASSERT
				assert_throws(jumper(target, 0), padding_insufficient);

				// INIT
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size + 8));
					fill(target - (c_jumper_size + 5), target, (byte)0x90);
					target[-(c_jumper_size + 5)] = 0x87;
					target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}

				// INIT / ACT / ASSERT
				assert_throws(jumper(target, 0), padding_insufficient);

				// INIT
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size + 8));
					fill(target - (c_jumper_size + 5), target, (byte)0x83);
					target[-1] = 0x12;
				}

				// INIT / ACT / ASSERT
				assert_throws(jumper(target, 0), padding_insufficient);
			}


			test( PaddingSpaceIsRestoredOnDestruction )
			{
				// INIT
				auto target = edge.get() + c_jumper_size;
				{
					scoped_unprotect u(byte_range(target - c_jumper_size, c_jumper_size));
					fill(target - c_jumper_size, target, (byte)0xC9);
					target[0] = 0x66, target[1] = 0x90; // xchg ax, ax
				}

				// ACT
				jumper(target, 0);

				// ASSERT
				assert_is_true(all_of(target - c_jumper_size, target, [] (byte b) {	return b == 0xC9;	}));

				// INIT
				target = edge.get() + c_jumper_size + 5;
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size + 8));
					fill(target - c_jumper_size - 5, target, (byte)0xFD);
					target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}

				// ACT
				jumper(target, 0);

				// ASSERT
				assert_is_true(all_of(target - c_jumper_size - 5, target, [] (byte b) {	return b == 0xFD;	}));
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
				{
					scoped_unprotect u(byte_range(target - c_jumper_size, c_jumper_size));
					fill(target - c_jumper_size, target, (byte)0xC9);
					target[0] = 0x66, target[1] = 0x90; // xchg ax, ax
				}
				{
					jumper j(target, address_cast_hack<void *>(&three));

				// ACT / ASSERT
					assert_equal("three", address_cast_hack<string (*)()>(target - c_jumper_size)());
				}

				// INIT / ACT
				target = edge.get() + c_jumper_size + 5;
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size));
					fill(target - c_jumper_size - 5, target, (byte)0xC9);
					target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}
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
				assert_is_true(j1.activate(true));
				assert_is_true(j2.activate(true));

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

				j1.activate(true);
				j2.activate(true);

				// ACT
				assert_is_true(j1.revert(true));
				assert_is_true(j2.revert(true));

				// ACT / ASSERT
				assert_equal("one", one());
				assert_equal("two", two());

				// ASSERT
				assert_is_false(j1.active());
				assert_is_false(j2.active());
			}


			test( DiversionEndsWhenJumperIsDestroyed )
			{
				// INIT
				{
					jumper j(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));

					j.activate(true);

				// ACT
				}

				// ACT / ASSERT
				assert_equal("one", one());
			}


			test( ConsequentApplicationDoesNothing )
			{
				// INIT
				{
					jumper j(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));

					j.activate(true);

				// ACT / ASSERT
					assert_is_false(j.activate(true));

				// ACT
				}

				// ASSERT
				assert_equal("one", one());
			}


			test( RevertingInactiveJumperDoesNothing )
			{
				// INIT
				{
					jumper j(address_cast_hack<void *>(&one), address_cast_hack<void *>(&three));

				// ACT / ASSERT
					assert_is_false(j.revert(true));

				// ACT
				}

				// ASSERT
				assert_equal("one", one());
			}


			test( AttemptToConstructOverSingleByteOpcodedFunctionFails )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 4 /*guarantee no problems with space to move to*/;

				// INIT / ACT / ASSERT
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0x52, target[1] = 0xC3;	} // push ..., ret
				assert_throws(jumper(target, 0), leading_too_short);
				{	scoped_unprotect u(byte_range(target, 2)); target[0] = 0xC3, target[1] = 0x52;	} // ret, push ...
				assert_throws(jumper(target, 0), leading_too_short);
			}


			test( NoRestorationTakesPlaceAfterDetachingJumper )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 5;
				vector<byte> before_detach;
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size + 8));
					fill(target - c_jumper_size - 5, target, (byte)0xFD);
					target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}
				unique_ptr<jumper> j(new jumper(target, 0));

				j->activate(false);
				copy(target - c_jumper_size - 5, target + 2, back_inserter(before_detach));

				// ACT
				j->detach();

				// ASSERT
				assert_is_false(j->active());

				// ACT
				j.reset();

				// ASSERT
				assert_equal(before_detach, vector<byte>(target - c_jumper_size - 5, target + 2));
			}


			test( NoMemoryLockOnDestroyAfterDetach )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 5;
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size + 8));
					fill(target - c_jumper_size - 5, target, (byte)0xFD);
					target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}
				unique_ptr<jumper> j(new jumper(target, 0));

				j->activate(false);
				j->detach();

				// ACT
				edge.reset(); // release memory

				// ACT / ASSERT (must not crash)
				j.reset();
			}


			test( ActivationAndRevertFailForADetachedJumper )
			{
				// INIT
				auto target = edge.get() + c_jumper_size + 5;
				{
					scoped_unprotect u(byte_range(target - c_jumper_size - 5, c_jumper_size + 8));
					fill(target - c_jumper_size - 5, target, (byte)0xFD);
					target[0] = 0x89, target[1] = 0x65, target[2] = 0xF0; // mov esp, dword ptr [ebp-0x10]
				}
				jumper j(target, 0);

				j.detach();

				// ACT / ASSERT
				assert_throws(j.activate(false), logic_error);
				assert_throws(j.revert(false), logic_error);
			}

		end_test_suite
	}
}
