cd 3rd\boost
call bootstrap.bat
bjam.exe address-model=32 --with-date_time --with-system --with-thread variant=release --with-filesystem link=static threading=multi runtime-link=static stage
bjam.exe address-model=32 ..\.. -d2 variant=release link=static threading=multi runtime-link=static
bjam.exe address-model=32 --with-date_time --with-system --with-thread variant=release --with-filesystem link=static threading=multi stage
bjam.exe address-model=32 --with-date_time --with-system --with-thread variant=debug --with-filesystem link=static threading=multi stage
cd ..\..
copy 3rd\openssl\lib\libeay32.dll bin\
copy 3rd\openssl\lib\ssleay32.dll bin\
rd /q /s dist
mkdir dist
mkdir dist\xixibase
mkdir dist\xixibase\bin
copy bin\xixibase.exe dist\xixibase\bin\
copy 3rd\openssl\lib\libeay32.dll dist\xixibase\bin\
copy 3rd\openssl\lib\ssleay32.dll dist\xixibase\bin\
mkdir dist\xixibase\conf
copy conf\web.xml dist\xixibase\conf\
copy conf\server.xml dist\xixibase\conf\
copy conf\privkey.pem dist\xixibase\conf\
copy conf\cacert.pem dist\xixibase\conf\
mkdir dist\xixibase\webapps
xcopy /E webapps dist\xixibase\webapps