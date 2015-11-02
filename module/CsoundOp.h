#include <boost/format.hpp>
#include <iostream>

using namespace std;
using boost::format;

namespace mt{

    /*
     *      最後のパラメタ0個の時にこれが呼ばれる。
     */
    string op_expander(boost::format& fmt) {
        return fmt.str() + "\n";
    }
    
    /*
     *      受け取った値を1個ずつ処理する
     */
    template<typename TPrm, typename... TPrms>
    string op_expander(boost::format& fmt, TPrm prm, TPrms... prms) {
        fmt % prm;
        return op_expander(fmt, prms...);
    }

    /*
     *      Variadic template
     *      どんな型の値を何個でも受け取って間に空白を挿入する。
     */
    template<typename... TPrms>
    string op( TPrms... prms) {
        int nPrm = sizeof...(prms);
        string fmtstr;
        for( int i=0; i<nPrm; i++)
            fmtstr += "%" + str( format("%d") % (i+1) ) + "% ";
        
        boost::format fmt(fmtstr);
        return op_expander(fmt, prms...);
    }
}