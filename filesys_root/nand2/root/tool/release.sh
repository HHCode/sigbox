cd /nand2
rm -rf ./root
mkdir ./root

cd ./nfs
tar -cf - ./ | tar -xf - -C ../root
