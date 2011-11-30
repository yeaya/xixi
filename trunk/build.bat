cd 3rd\boost
call bootstrap.bat
bjam.exe --with-date_time --with-system --with-thread variant=release --with-filesystem link=static threading=multi runtime-link=static stage
bjam.exe ..\.. -d2 variant=release link=static threading=multi runtime-link=static
bjam.exe --with-date_time --with-system --with-thread variant=release --with-filesystem link=static threading=multi stage
bjam.exe --with-date_time --with-system --with-thread variant=debug --with-filesystem link=static threading=multi stage
cd ..\..
rd /q /s dist
mkdir dist
mkdir dist\xixibase
mkdir dist\xixibase\bin
copy bin\xixibase.exe dist\xixibase\bin\
mkdir dist\xixibase\conf
copy conf\web.xml dist\xixibase\conf\