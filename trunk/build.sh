xixibaseDIR=$(pwd)
cd 3rd/boost 
./bootstrap.sh
./bjam --with-date_time --with-system --with-thread variant=release link=static threading=multi runtime-link=static stage
./bjam $xixibaseDIR -d2 variant=release link=static threading=multi runtime-link=static    
cd $xixibaseDIR