
function mysave (filename, mname)
    m = evalin('caller', mname);
    f = fopen(filename, 'w');
    fprintf(f, '# written by matlab mysave\n# name: %s\n# type: matrix\n# rows: %d\n# columns: %d\n', mname, size(m, 1), size(m, 2));
    fclose(f);
    
    save(filename, 'm', '-ascii', '-append')
    
    
