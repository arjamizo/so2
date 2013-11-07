mkdir a a/b a/b/c a/d e e/f a/d/g
chmod -w -R e/f 
chmod u-x a/d

find . 
