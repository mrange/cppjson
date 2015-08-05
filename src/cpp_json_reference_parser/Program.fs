open System
open System.Globalization
open System.IO
open System.Text
open System.Threading

open Microsoft.FSharp.Core.Printf

open MiniJson.JsonModule

[<EntryPoint>]
let main argv = 
  Thread.CurrentThread.CurrentCulture   <- CultureInfo.InvariantCulture
  Thread.CurrentThread.CurrentUICulture <- CultureInfo.InvariantCulture

  try
    Environment.CurrentDirectory <- AppDomain.CurrentDomain.BaseDirectory

    let testCases =
      Directory.GetFiles (@"../../../../test_cases/", @"*.json")
      |> Array.map (fun path -> Path.GetFileName path, Path.GetFullPath path)


    for testCase, testCasePath in testCases do
      printfn "Processing: %s" testCase
      let sb                = StringBuilder ()
      let app (s : string)  = ignore <| sb.AppendLine s; true
      let appf f            = kprintf app f
      let v =
        { new IParseVisitor with
            member x.NullValue    ()        =           app   "NullValue        : null"
            member x.BoolValue    b         =           appf  "BoolValue        : %s" (if b then "true" else "false")
            member x.NumberValue  n         =           appf  "NumberValue      : %f" n
            member x.StringValue  s         =           appf  "StringValue      : %s" (s.ToString ()) // TODO: Escape
            member x.ArrayBegin   ()        =           app   "Array            : begin"
            member x.ArrayEnd     ()        =           app   "Array            : end"
            member x.ObjectBegin  ()        =           app   "Object           : begin"
            member x.ObjectEnd    ()        =           app   "Object           : end"
            member x.MemberKey    mk        =           appf  "MemberKey        : %s" <| mk.ToString ()
            member x.ExpectedChar (pos, ch) = ignore <| appf  "ExpectedChar     : %d, %s" pos (ch.ToString ()) // TODO: Escape
            member x.Expected     (pos, tk) = ignore <| appf  "ExpectedToken    : %d, %s" pos tk  // TODO: Escape
            member x.Unexpected   (pos, tk) = ignore <| appf  "UnexpectedToken  : %d, %s" pos tk  // TODO: Escape
        }

      let json        = File.ReadAllText testCasePath
      let mutable pos = 0
      let result      = tryParse v json &pos
      ignore <| appf "ParsePosition    : %d" pos
      ignore <| appf "ParseResult      : %s" (if result then "true" else "false")
      File.WriteAllText (testCasePath + ".result", sb.ToString ())

    printfn "Done"
    0
  with
  | e -> 999
