curr_work_path=`pwd`

source_dir=`pwd`

build_dir_path="$curr_work_path/build"
rm -r -f ${build_dir_path}
mkdir ${build_dir_path}

install_dir_path="$build_dir_path/install"
rm -r -f ${install_dir_path}
mkdir ${install_dir_path}

cd ${build_dir_path}

cmake -DCMAKE_INSTALL_PREFIX=${install_dir_path} ${source_dir}
make install

bin_path=${build_dir_path}/bin/
for file in ${bin_path}/*
do 
    if test -f ${file}
    then
        ${file}
    fi
done
