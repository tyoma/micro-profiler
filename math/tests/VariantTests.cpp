#include <math/variant.h>

#include <string>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		namespace
		{
			struct complex_t
			{
				complex_t(int v0, double v1, bool v2)
					: i(v0), d(v1), b(v2)
				{	}

				int i;
				double d;
				bool b;
			};

			struct visitor1cv : static_visitor<>
			{
				void operator ()(const int &) const {	type = 0;	}
				void operator ()(const double &) const {	type = 1;	}
				void operator ()(const bool &) const {	type = 2;	}

				mutable int type;
			};

			struct visitor1ci : static_visitor<int>
			{
				int operator ()(const int &) const {	return 0;	}
				int operator ()(const double &) const {	return 1;	}
				int operator ()(const bool &) const {	return 2;	}
			};

			struct visitor1c : static_visitor<complex_t>
			{
				complex_t operator ()(const int &v) const {	return complex_t(v, 0.0, false);	}
				complex_t operator ()(const double &v) const {	return complex_t(0, v, false);	}
				complex_t operator ()(const bool &v) const {	return complex_t(0, 0.0, v);	}

				mutable int type;
			};

			struct visitor1v : static_visitor<>
			{
				void operator ()(int &) const {	type = 0;	}
				void operator ()(double &) const {	type = 1;	}
				void operator ()(bool &) const {	type = 2;	}

				mutable int type;
			};

			struct visitor1i : static_visitor<int>
			{
				int operator ()(int &) const {	return 0;	}
				int operator ()(double &) const {	return 1;	}
				int operator ()(bool &) const {	return 2;	}
			};

			struct visitor1 : static_visitor<complex_t>
			{
				complex_t operator ()(int &v) const {	return complex_t(v, 0.0, false);	}
				complex_t operator ()(double &v) const {	return complex_t(0, v, false);	}
				complex_t operator ()(bool &v) const {	return complex_t(0, 0.0, v);	}

				mutable int type;
			};
		}

		begin_test_suite( VariantTests )
			test( TypeIdDependsOnTheTypeUsedForConstruction )
			{
				// INIT / ACT
				variant<int> v1(123);
				variant<bool> v2(true);
				variant< int, variant<double> > v30(123), v31(21.11);
				variant< int, variant< double, variant<bool> > > v40(123), v41(21.11), v42(false);
				variant< string, variant< int, variant< double, variant<bool> > > > v50(123), v51(21.11), v52(false), v53((string)"");

				// ACT / ASSERT
				assert_equal(0u, v1.get_type());
				assert_equal(0u, v2.get_type());
				assert_equal(1u, v30.get_type());
				assert_equal(0u, v31.get_type());
				assert_equal(2u, v40.get_type());
				assert_equal(1u, v41.get_type());
				assert_equal(0u, v42.get_type());
				assert_equal(2u, v50.get_type());
				assert_equal(1u, v51.get_type());
				assert_equal(0u, v52.get_type());
				assert_equal(3u, v53.get_type());
			}


			test( TypeSetIsLaterVisitedConst )
			{
				// INIT
				const variant< int, variant< double, variant<bool> > > v10(123);
				const variant< int, variant< double, variant<bool> > > v101(723);
				const variant< int, variant< double, variant<bool> > > v11(21.11);
				const variant< int, variant< double, variant<bool> > > v111(121.11);
				const variant< int, variant< double, variant<bool> > > v12(false);
				const variant< int, variant< double, variant<bool> > > v121(true);
				visitor1cv vv;
				visitor1ci vi;
				visitor1c v;

				// ACT / ASSERT
				v10.visit(vv);
				assert_equal(0, vv.type);
				v11.visit(vv);
				assert_equal(1, vv.type);
				v12.visit(vv);
				assert_equal(2, vv.type);

				assert_equal(0, v10.visit(vi));
				assert_equal(1, v11.visit(vi));
				assert_equal(2, v12.visit(vi));
				assert_equal(123, v10.visit(v).i);
				assert_equal(723, v101.visit(v).i);
				assert_equal(21.11, v11.visit(v).d);
				assert_equal(121.11, v111.visit(v).d);
				assert_equal(false, v12.visit(v).b);
				assert_equal(true, v121.visit(v).b);
			}


			test( TypeSetIsLaterVisitedNonConst )
			{
				// INIT
				variant< int, variant< double, variant<bool> > > v10(123), v101(723), v11(21.11), v111(31.11), v12(false), v121(true);
				visitor1v vv;
				visitor1i vi;
				visitor1c v;

				// ACT / ASSERT
				v10.visit(vv);
				assert_equal(0, vv.type);
				v11.visit(vv);
				assert_equal(1, vv.type);
				v12.visit(vv);
				assert_equal(2, vv.type);

				assert_equal(0, v10.visit(vi));
				assert_equal(1, v11.visit(vi));
				assert_equal(2, v12.visit(vi));
				assert_equal(123, v10.visit(v).i);
				assert_equal(723, v101.visit(v).i);
				assert_equal(21.11, v11.visit(v).d);
				assert_equal(31.11, v111.visit(v).d);
				assert_equal(false, v12.visit(v).b);
				assert_equal(true, v121.visit(v).b);
			}


			test( TypeChangedIsLaterVisited )
			{
				// INIT
				variant< double, variant< bool, variant<int> > > vt(130.0);
				visitor1ci vi;
				visitor1c v;

				// ACT
				vt.change_type(2);

				// ASSERT
				assert_equal(1, vt.visit(vi));
				assert_equal(130.0, vt.visit(v).d);

				// ACT
				vt.change_type(0);

				// ASSERT
				assert_equal(0, vt.visit(vi));

				// ACT
				vt.change_type(1);

				// ASSERT
				assert_equal(2, vt.visit(vi));
			}


			test( VariantsOfDifferentControlledTypesAreNotEqual )
			{
				// ACT / ASSERT
				assert_is_false((variant< double, variant<int> >(0.0) == variant< double, variant<int> >(0)));
				assert_is_false((variant< double, variant<bool> >(false) == variant< double, variant<bool> >(0.0)));
				assert_is_false((variant< string, variant< double, variant<bool> > >(false)
					== variant< string, variant< double, variant<bool> > >(string())));
				assert_is_true((variant< double, variant<int> >(0.0) != variant< double, variant<int> >(0)));
				assert_is_true((variant< double, variant<bool> >(false) != variant< double, variant<bool> >(0.0)));
				assert_is_true((variant< string, variant< double, variant<bool> > >(false)
					!= variant< string, variant< double, variant<bool> > >(string())));
			}


			test( VariantsOfDifferentControlledValuesAreNotEqual )
			{
				// ACT / ASSERT
				assert_is_false((variant< double, variant<int> >(10.0) == variant< double, variant<int> >(13.1)));
				assert_is_false((variant< double, variant<bool> >(false) == variant< double, variant<bool> >(true)));
				assert_is_false((variant< bool, variant< double, variant<string> > >(string("lorem"))
					== variant< bool, variant< double, variant<string> > >(string("ipsum"))));
				assert_is_true((variant< double, variant<int> >(10.0) != variant< double, variant<int> >(13.1)));
				assert_is_true((variant< double, variant<bool> >(false) != variant< double, variant<bool> >(true)));
				assert_is_true((variant< bool, variant< double, variant<string> > >(string("lorem"))
					!= variant< bool, variant< double, variant<string> > >(string("ipsum"))));
			}


			test( VariantsOfSameControlledValuesAreEqual )
			{
				// ACT / ASSERT
				assert_is_true((variant< double, variant<int> >(13) == variant< double, variant<int> >(13)));
				assert_is_true((variant< double, variant<bool> >(13.1) == variant< double, variant<bool> >(13.1)));
				assert_is_true((variant< bool, variant< double, variant<string> > >(string("lorem"))
					== variant< bool, variant< double, variant<string> > >(string("lorem"))));
				assert_is_true((variant< bool, variant< double, variant<string> > >(true)
					== variant< bool, variant< double, variant<string> > >(true)));

				assert_is_false((variant< double, variant<int> >(13) != variant< double, variant<int> >(13)));
				assert_is_false((variant< double, variant<bool> >(13.1) != variant< double, variant<bool> >(13.1)));
				assert_is_false((variant< bool, variant< double, variant<string> > >(string("lorem"))
					!= variant< bool, variant< double, variant<string> > >(string("lorem"))));
				assert_is_false((variant< bool, variant< double, variant<string> > >(true)
					!= variant< bool, variant< double, variant<string> > >(true)));
			}
		end_test_suite
	}
}
