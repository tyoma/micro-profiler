#include <sqlite++/database.h>

#include "helpers.h"

#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace sql
	{
		namespace tests
		{
			using namespace micro_profiler::tests;

			namespace
			{
				struct movie
				{
					int id;
					string name;
					int year;

					static movie make(int id_, string name_, int year_)
					{
						movie r = {	id_, name_, year_	};
						return r;
					}

					bool operator <(const movie &rhs) const
					{	return make_tuple(id, name, year) < make_tuple(rhs.id, rhs.name, rhs.year);	}
				};

				struct actor
				{
					int id;
					string name;
					int date_of_birth;

					static actor make(int id_, string name_, int date_of_birth_)
					{
						actor r = {	id_, name_, date_of_birth_	};
						return r;
					}

					bool operator <(const actor &rhs) const
					{	return make_tuple(id, name, date_of_birth) < make_tuple(rhs.id, rhs.name, rhs.date_of_birth);	}
				};

				struct character
				{
					int movie_id, actor_id;
					string name;

					static character make(int movie_id_, int actor_id_, string name_)
					{
						character r = {	movie_id_, actor_id_, name_	};
						return r;
					}

					bool operator <(const character &rhs) const
					{	return make_tuple(movie_id, actor_id, name) < make_tuple(rhs.movie_id, rhs.actor_id, rhs.name);	}
				};



				template <typename V>
				void describe(V &visitor, movie *)
				{
					visitor("movies");
					visitor(pk(&movie::id), "id");
					visitor(&movie::name, "name");
					visitor(&movie::year, "year");
				}

				template <typename V>
				void describe(V& visitor, actor *)
				{
					visitor("actors");
					visitor(pk(&actor::id), "id");
					visitor(&actor::name, "name");
					visitor(&actor::date_of_birth, "dob");
				}

				template <typename V>
				void describe(V& visitor, character *)
				{
					visitor("roles");
					visitor(&character::movie_id, "movie_id");
					visitor(&character::actor_id, "actor_id");
					visitor(&character::name, "name");
				}
			}

			begin_test_suite( JoiningTests )
				temporary_directory dir;
				string path;
				unique_ptr<transaction> tx;
				vector<movie> movies;
				vector<actor> actors;
				vector<character> characters;

				init( Init )
				{
					path = dir.track_file("sample-db.db");
					tx.reset(new transaction(create_connection(path.c_str())));
					tx->create_table<movie>();
					tx->create_table<actor>();
					tx->create_table<character>();

					movies = plural
						+ movie::make(0, "Heat", 1995)
						+ movie::make(0, "The Godfather Part II", 1974)
						+ movie::make(0, "Top Gun", 1986);
					write_all(*tx, movies);

					actors = plural
						+ actor::make(0, "Al Pacino", 19400425)
						+ actor::make(0, "Robert De Niro", 19430817)
						+ actor::make(0, "Val Kilmer", 19591231)
						+ actor::make(0, "Tom Cruise", 19620703);
					write_all(*tx, actors);

					characters = plural
						+ character::make(movies[0].id, actors[0].id, "Lt. Vincent Hanna")
						+ character::make(movies[0].id, actors[1].id, "Neil McCauley")
						+ character::make(movies[0].id, actors[2].id, "Chris Shiherlis")
						+ character::make(movies[1].id, actors[0].id, "Michael Corleone")
						+ character::make(movies[1].id, actors[1].id, "Vito Corleone")
						+ character::make(movies[2].id, actors[2].id, "Maverick")
						+ character::make(movies[2].id, actors[3].id, "Ice");
					write_all(*tx, characters);
				}


				test( UnrelatedSelectsReturnCartesianSets )
				{
					// INIT / ACT
					auto moviesXactors = read_all(tx->select< tuple<movie, actor> >());
					auto actorsXmovies = read_all(tx->select< tuple<actor, movie> >());
					auto moviesXactorsXroles = read_all(tx->select< tuple<movie, actor, character> >());

					// ASSERT
					assert_equivalent(plural
						+ make_tuple(movies[0], actors[0])
						+ make_tuple(movies[0], actors[1])
						+ make_tuple(movies[0], actors[2])
						+ make_tuple(movies[0], actors[3])
						+ make_tuple(movies[1], actors[0])
						+ make_tuple(movies[1], actors[1])
						+ make_tuple(movies[1], actors[2])
						+ make_tuple(movies[1], actors[3])
						+ make_tuple(movies[2], actors[0])
						+ make_tuple(movies[2], actors[1])
						+ make_tuple(movies[2], actors[2])
						+ make_tuple(movies[2], actors[3]), moviesXactors);
					assert_equivalent(plural
						+ make_tuple(actors[0], movies[0])
						+ make_tuple(actors[1], movies[0])
						+ make_tuple(actors[2], movies[0])
						+ make_tuple(actors[3], movies[0])
						+ make_tuple(actors[0], movies[1])
						+ make_tuple(actors[1], movies[1])
						+ make_tuple(actors[2], movies[1])
						+ make_tuple(actors[3], movies[1])
						+ make_tuple(actors[0], movies[2])
						+ make_tuple(actors[1], movies[2])
						+ make_tuple(actors[2], movies[2])
						+ make_tuple(actors[3], movies[2]), actorsXmovies);

					vector< tuple<movie, actor, character> > reference;
					for (auto i = begin(movies); i != end(movies); ++i)
						for (auto j = begin(actors); j != end(actors); ++j)
							for (auto k = begin(characters); k != end(characters); ++k)
								reference.push_back(make_tuple(*i, *j, *k));

					assert_equivalent(reference, moviesXactorsXroles);
				}


				test( JoiningExpressionsAreSupported )
				{
					// INIT / ACT
					auto movieCharacters = read_all(tx->select< tuple<movie, character> >(c<0>(&movie::id) == c<1>(&character::movie_id)));

					// ASSERT
					assert_equivalent(plural
						+ make_tuple(movies[0], characters[0])
						+ make_tuple(movies[0], characters[1])
						+ make_tuple(movies[0], characters[2])
						+ make_tuple(movies[1], characters[3])
						+ make_tuple(movies[1], characters[4])
						+ make_tuple(movies[2], characters[5])
						+ make_tuple(movies[2], characters[6]), movieCharacters);

					// INIT / ACT
					auto actorsByMovies = read_all(tx->select< tuple<actor, movie, character> >(
						c<0>(&actor::id) == c<2>(&character::actor_id) && c<1>(&movie::id) == c<2>(&character::movie_id)
					));

					// ASSERT
					assert_equivalent(plural
						+ make_tuple(actors[0], movies[0], characters[0])
						+ make_tuple(actors[1], movies[0], characters[1])
						+ make_tuple(actors[2], movies[0], characters[2])
						+ make_tuple(actors[0], movies[1], characters[3])
						+ make_tuple(actors[1], movies[1], characters[4])
						+ make_tuple(actors[2], movies[2], characters[5])
						+ make_tuple(actors[3], movies[2], characters[6]), actorsByMovies);

					// INIT / ACT
					auto AlPacinoMovies = read_all(tx->select< tuple<actor, movie, character> >(
						c<0>(&actor::id) == c<2>(&character::actor_id) && c<1>(&movie::id) == c<2>(&character::movie_id)
							&& c<0>(&actor::name) == p<const string>("Al Pacino")
					));

					// ASSERT
					assert_equivalent(plural
						+ make_tuple(actors[0], movies[0], characters[0])
						+ make_tuple(actors[0], movies[1], characters[3]), AlPacinoMovies);
				}
			end_test_suite
		}
	}
}
