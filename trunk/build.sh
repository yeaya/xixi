cd 3rd/boost/
./bootstrap.sh
./bjam --with-date_time --with-system --with-thread variant=release link=static threading=multi stage 
./bjam ../.. -d2 variant=release link=static threading=multi 
cd ../..