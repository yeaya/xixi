cd 3rd\boost
call bootstrap.bat
bjam.exe --with-date_time --with-system --with-thread variant=release --with-filesystem link=static threading=multi runtime-link=static stage
bjam.exe ..\.. -d2 variant=release link=static threading=multi runtime-link=static
bjam.exe --with-date_time --with-system --with-thread variant=release --with-filesystem link=static threading=multi stage
bjam.exe --with-date_time --with-system --with-thread variant=debug --with-filesystem link=static threading=multi stage
cd ..\..