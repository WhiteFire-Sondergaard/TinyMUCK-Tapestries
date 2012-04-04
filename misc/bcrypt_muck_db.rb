#!/usr/bin/env ruby

require 'optparse'
require 'pp'
require 'rubygems'
require 'progressbar'
require 'json'
require 'bcrypt'

$options = {}
OptionParser.new do |opts|
	opts.banner = "Usage: bcrypt_muck_db.rb [options]"

	opts.separator ""
	opts.separator "Specific options:"

	opts.on("--db DATABASE", "Specify the DATABASE to convert.") do |db|
		$options[:db] = File.new(db, "r")
	end

   opts.on("--out DATABASE", "Specify the output DATABASE.") do |db|
      raise "Will not overwrite #{db}" if File.exist?(db)
      $options[:out] = File.new(db, "w")
   end

   opts.on("--cache CACHEFILE", "Specify the in/out CACHEFILE.") do |filename|
      $options[:cache] = filename
   end

end.parse!

raise "No database given." if $options[:db].class != File

module FuzzBallDB
   BCRYPT_COST = 10

	DUMP_HEADER = "***Foxen5 TinyMUCK DUMP Format***"
   DUMP_FOOTER = "***END OF DUMP***"
	DB_PARMSINFO = 0x0001
   DB_COMPRESSED = 0x0002
   PROP_START = "*Props*"
   PROP_END = "*End*"

   TYPE_ROOM = 0x0
   TYPE_THING = 0x1
   TYPE_EXIT = 0x2
   TYPE_PLAYER = 0x3
   TYPE_PROGRAM = 0x4
   TYPE_GARBAGE = 0x6
   TYPE_MASK = 0x7

   class MuObject
      def initialize(dbref)
         def db_readline(echo = true) 
            FuzzBallDB::db_readline(echo)
         end
         @dbref = dbref
         @name = db_readline
         @location = db_readline
         @contents_head = db_readline.to_i
         @next = db_readline.to_i
         @flags = db_readline.to_i
         @type = @flags & TYPE_MASK
         @created = db_readline.to_i
         @lastused = db_readline.to_i
         @usecount = db_readline.to_i
         @modified = db_readline.to_i

         # Skip properties
         # puts "Properties..."
         l = db_readline
         raise "Unexpected line #{l}; expecting #{PROP_START}" if l != PROP_START
         while db_readline != PROP_END do end

         # puts "Object Stuff..."
         # Object type specific stuff...
         case @type
         when TYPE_THING
            @home = db_readline.to_i
            @exits = db_readline.to_i
            @owner = db_readline.to_i
            @value = db_readline.to_i

         when TYPE_ROOM
            @dropto = db_readline.to_i     
            @exits = db_readline.to_i
            @owner = db_readline.to_i

         when TYPE_EXIT
            @ndest = db_readline.to_i
            @dest = Array.new()
            for i in (1..@ndest)
               @dest[i-1] = db_readline.to_i
            end
            @owner = db_readline.to_i

         when TYPE_PROGRAM
            @owner = db_readline.to_i

         when TYPE_PLAYER
            @home = db_readline.to_i
            @exits = db_readline.to_i
            @value = db_readline.to_i

            #
            # Player passwords. Magic happens here
            #
            @password = db_readline(false)
            key = "#{@dbref}:#{@created}:#{@password}"
            cache = FuzzBallDB::get_cache
            if cache[key]
               @password = cache[key]
            else
               @password = BCrypt::Password.create(@password, :cost => BCRYPT_COST)
               cache[key] = @password
            end
            
            FuzzBallDB::db_writeline("#{@password}\n")
            # End Magic
         end # case
      end
   end

   def self.db_readline(echo = true)
      line = @@db.readline
      @@output.write(line) if (echo && @@output.class == File)
      line.chomp
   end

   def self.db_writeline(line)
      @@output.write(line) if @@output.class == File
   end

   def self.get_cache
      @@cache
   end

   def self.load(db, output, cache = 'bcrypt_cache.json')

      @@db = db
      @@output = output
      if File.exist?(cache)
         puts "Loading cache."
         @@cache = JSON.parse(File.new(cache, "r").read)
      else
         @@cache = {} # this should be loaded if cache exists.
      end

      if (db_readline.chomp != DUMP_HEADER) then raise "Does not appear to be a Foxen 5 dump." end

      objects = db_readline.to_i
      db_flags = db_readline.to_i

      # Read the parms
      if db_flags & DB_PARMSINFO then
         parms = {}
         num_parms = db_readline.to_i
         for i in (1..num_parms)
            l = db_readline
         end
      end

      # Skip the compression data
      if db_flags & DB_COMPRESSED then
         for i in (1..4096)
            l = db_readline
         end
      end

      puts "Loading #{objects} objects..."

      bar = ProgressBar.new('bcrypting', objects)

      while (l = db_readline) != DUMP_FOOTER do
         if (l[0..0] != '#') then raise "This is not the start of an object: #{l}" end
         dbref = l[1..-1].to_i

         #puts "Object #{dbref}..."

         MuObject.new(dbref)

         bar.inc
      end

      bar.finish

      puts "Saving Cache..."
      File.open(cache, "w").write(@@cache.to_json)
   end

end

FuzzBallDB.load($options[:db], $options[:out], $options[:cache])
