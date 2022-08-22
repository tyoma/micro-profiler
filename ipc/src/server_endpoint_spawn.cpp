#include <ipc/endpoint_spawn.h>

#ifdef WIN32
	#include <fcntl.h>
	#include <io.h>
#endif

using namespace micro_profiler;
using namespace micro_profiler::ipc;
using namespace std;

int main(int argc, const char *argv[])
{
	class pipe_channel : public channel
	{
	public:
		pipe_channel(FILE *stream)
			: _stream(stream)
		{	}

	private:
		virtual void disconnect() throw()
		{	}

		virtual void message(const_byte_range payload)
		{
			auto size = static_cast<unsigned int>(payload.length());

			fwrite(&size, sizeof size, 1, _stream);
			fwrite(payload.begin(), 1, payload.length(), _stream);
			fflush(_stream);
		}

	private:
		FILE *const _stream;
	};

#ifdef WIN32
	_setmode(_fileno(stdin), O_BINARY);
	_setmode(_fileno(stdout), O_BINARY);
#endif

	unsigned int size;
	vector<byte> buffer;
	vector<string> arguments;
	pipe_channel outbound(stdout);

	for (auto i = 1; i != argc; ++i)
		arguments.push_back(argv[i]);

	const auto instance = spawn::create_session(arguments, outbound);

	while (fread(&size, sizeof size, 1, stdin))
	{
		buffer.resize(size);
		fread(buffer.data(), 1, size, stdin); // TODO: check for abruption here
		instance->message(const_byte_range(buffer.data(), size));
	}
	instance->disconnect();
	return 0;
}
