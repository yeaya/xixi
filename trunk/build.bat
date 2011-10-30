cd 3rd\boost
call bootstrap.bat
bjam.exe --with-date_time --with-system --with-thread variant=release link=static threading=multi runtime-link=static stage
bjam ..\.. -d2 variant=release link=static threading=multi runtime-link=static
cd ..\..