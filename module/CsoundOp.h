#include <boost/format.hpp>
#include <iostream>

using namespace std;
using boost::format;

namespace mt{

    /*
     *      private use only
     */
    string op(boost::format& fmt) {
        return fmt.str() + "\n";
    }
    
    /*
     *      With user defined boost::format
     *
     *      Usage
     *      boost::format fmt("i %d, %d, %d");
     *      cout << mt::op( fmt, 1,2,3 );
     */
    template<typename TPrm, typename... TPrms>
    string op(boost::format& fmt, TPrm prm, TPrms... prms) {
        fmt % prm;
        return op(fmt, prms...);
    }

    /*
     *      Usage
     *      cout << mt::op( 1,2,3 );
     */
    template<typename... TPrms>
    string op( TPrms... prms) {
        int nPrm = sizeof...(prms);
        string fmtstr;
        for( int i=0; i<nPrm; i++)
            fmtstr += "%" + to_string( i+1 ) + "%" + " ";
        
        boost::format fmt(fmtstr);
        return op(fmt, prms...);
    }
}