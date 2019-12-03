


Install vcpkg then install the following packages: 1 - 
vcpkg install boost:x64-windows-static 2 - 
vckpg install cryptopp:x64-windows-static

go to project properties -> C/C++ -> Code Generation -> Runtime Library and change that else you will get 
a lot of errors.

then go to 
vcpkg\scripts\buildsystems\msbuild

and download and edit your vcpkg.targets with the one in here.

after replacing run vcpkg integrate install

it'll configure visual studio so u can use cryptopp bundled in the .exe file
and u wont generate dll files which can lead to easier cracking

for my sql part just look at customers.sql and you will find 
the table code.


Credits goes to me and greg(for helping me with static builds errors)

Discord if you need any help : Frankoo#8837 & Greg#0003
