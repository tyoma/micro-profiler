#include <logger/log.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			struct logger : log::logger
			{
				virtual void begin(const char *message, log::level level_) throw()
				{
					buffer += "+";
					buffer += level_ == log::info ? "i" : level_ == log::severe ? "S" : "?";
					buffer += " ";
					buffer += message;
				}

				virtual void add_attribute(const log::attribute &a_) throw()
				{
					log::buffer_t b;
					
					a_.format_value(b);
					_attributes.push_back(make_pair(a_.name, string(b.begin(), b.end())));
				}

				virtual void commit() throw()
				{
					if (!_attributes.empty())
						buffer += " (";
					for (auto i = _attributes.begin(); i != _attributes.end(); ++i)
					{
						if (i != _attributes.begin())
							buffer += ",";
						buffer += i->first;
						buffer += ":";
						buffer += i->second;
					}
					if (!_attributes.empty())
						buffer += ")";
					buffer += "-";
					_attributes.clear();
				}

				string buffer;

			private:
				vector< pair<string, string> > _attributes;
			};
		}

		begin_test_suite( LogEventTests )
			unique_ptr<mocks::logger> logger;

			init( Init )
			{
				logger.reset(new mocks::logger);
			}


			test( LogEventIsOkWithNullLogger )
			{
				// INIT / ACT / ASSERT
				log::e(nullptr, "a couple of ints") % 11 % -159;
			}


			test( PlainMessagesArePutToSinkInBeginCommitOrder )
			{
				// ACT
				log::e(logger.get(), "Lorem ipsum");

				// ASSERT
				assert_equal("+i Lorem ipsum-", logger->buffer);

				// ACT
				log::e(logger.get(), "dolor amet", log::severe);

				// ASSERT
				assert_equal("+i Lorem ipsum-+S dolor amet-", logger->buffer);
			}


			test( AnnonymousAttributesAreAddedToSink )
			{
				// ACT
				log::e(logger.get(), "a couple of ints") % 11 % -159;

				// ASSERT
				assert_equal("+i a couple of ints (:11,:-159)-", logger->buffer);

				// INIT
				logger->buffer.clear();

				// ACT
				log::e(logger.get(), "a couple of strings") % string("some string") % "some string literal";

				// ASSERT
				assert_equal("+i a couple of strings (:some string,:some string literal)-", logger->buffer);

				// INIT
				logger->buffer.clear();

				// ACT
				log::e(logger.get(), "another couple of strings") % string("abcdef") % "";

				// ASSERT
				assert_equal("+i another couple of strings (:abcdef,:)-", logger->buffer);

				// INIT
				logger->buffer.clear();

				// ACT
				log::e(logger.get(), "empty string") % (const char *)0;

				// ASSERT
				assert_equal("+i empty string (:<null>)-", logger->buffer);
			}
		end_test_suite
	}
}
