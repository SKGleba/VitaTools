(
cd kplugin/;
cmake ./ && make;
mv softpatch.skprx ../kernel.skprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm softpatch.velf && rm softpatch.skprx.out && rm softpatch;
cd ../uplugin/;
cmake ./ && make;
mv stor_format.suprx ../user.suprx;
rm -rf CMakeFiles && rm cmake_install.cmake && rm CMakeCache.txt && rm Makefile;
rm system_settings.xml.o && rm user && rm user.velf && rm stor_format.suprx.out;
cd ../;
make;
rm font.o && rm graphics.o && rm main.o && rm storageFormat.elf && rm storageFormat.velf;
rm eboot.bin && rm user.suprx && rm kernel.skprx && rm param.sfo;
echo "";
echo "DONE!";
echo "";
)