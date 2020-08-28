Class List inherits IO { 
  isEmpty() : Bool { { abort(); true; } };
  get_value() : Int{{abort(); new Int;}};
  get_next(): List{{ abort(); new List; }};
  sort() : List {get_next()};
  insert(i : Int) : List { get_next() };
  print_(counter : Int) : Object { abort() };
};

Class ListNode inherits List {
  val : Int;  
  next : List; 
  is_Empty :Bool <- true;
  isEmpty() : Bool { is_Empty};
  init__(head : Int, tail : List) :ListNode {
    {
      is_Empty <- false;
      val <- head;
      next <- tail;
      self;
    }
  };
    get_value() : Int{val};
    get_next(): List{next};
    sort() : List { if(isEmpty()) then self
              else (next.sort()).insert(val)
                    fi };

  insert(data : Int) : List {
    if(true = isEmpty()) then (new ListNode).init__(data,self)
    else
      if data < val then
          (new ListNode).init__(data,self)
      else
          (new ListNode).init__(val,next.insert(data))
      fi
    fi
  };
  print_(counter : Int) : Object {
    {
        if(true = isEmpty()) then true
        else{
         if (1 = counter) then out_string("The sorted list is:\n")
         else true
         fi;
         out_string("[");
         out_int(counter);
         out_string("]  ");
         out_int(val);
         out_string("\n");
         counter <- counter + 1; 
         next.print_(counter);
        }
        fi;
    }
  };
  

};

class Main inherits IO {
  list : List;
    length : Int;
    get_list() : List {
      {
    length <-in_int();
    list <- new ListNode;
    (let counter : Int <- 1 in
       while counter <= length
       loop 
         {
                 list <- (new ListNode).init__(in_int(),list);
           counter <- counter + 1;
         } 
       pool
    );
    list;
      }
  };    

  main() : Object {
    {
        out_string("Enter size of your input list and then Enter your list element by element:\n");
        list <- get_list().sort();
        let counter : Int <-2, head : Int <- list.get_value(), repeat_flag : Int <- 0, result: Int <- 1 in {        
          while counter <= length loop {
            list <- list.get_next();
            if (list.get_value() = head) then
              repeat_flag <- 1
            else
            {
              result <- result + 1;
              head <- list.get_value();
            }  
            fi;
            counter <- counter + 1;
          }
          pool;
          out_string("Number of unique elements:\n");
          out_int(result);
          out_string("\n");
        };
    }
  };
};
