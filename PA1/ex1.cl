class Main inherits IO {
    openpar_counter : Int <- 0;
    closepar_counter : Int <- 0; 
    counter: Int <- 0;
    flag: Int <- 0;
    balanced(inp_str : String): Bool {
    {
    while counter < inp_str.length() loop
    {
         if inp_str.substr(counter, 1) = "("  then 
            openpar_counter <- openpar_counter + 1 
         else 
                closepar_counter <- closepar_counter +1
         fi;
         
         if openpar_counter < closepar_counter then 
          {
          flag <- 1;
          counter <- counter + 1;
          }
         else counter <- counter + 1
         fi;
    }
    pool;
    if openpar_counter = closepar_counter  then
        if 0 = flag then
        	true
        else
        	false
        fi
    else
        false
    fi;
    }
    };

    main() : Object {
  	{
      out_string("Enter a string:\n");
      if balanced(in_string())
      then out_string("Yes\n")
      else out_string("No\n")
      fi;
  	}
    };
};
